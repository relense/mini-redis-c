#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include "resp-protocol.h"
#include "byte-buffer.h"
#include "commands.h"

#define CONNECTIONS_WAITING 10

void resolve_server_address(struct addrinfo* hints, int* get_addr_info_status, struct addrinfo** server_info) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;

    if((*get_addr_info_status = getaddrinfo(NULL, "8080", hints, server_info)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(*get_addr_info_status));
        exit(1);
    }
}

void create_listening_socket(struct addrinfo* server_info, int* socket_file_descriptor) {
    int reuse_addr = 1;
    struct addrinfo* addr;

    for(addr = server_info; addr != NULL; addr = addr->ai_next) {
        if((*socket_file_descriptor = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if(setsockopt(*socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(1);
        }

        if(bind(*socket_file_descriptor, addr->ai_addr, addr->ai_addrlen) == -1) {
            close(*socket_file_descriptor);
            perror("server: bind");
            continue;
        }

        break;
    }

    if(addr == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(server_info);
}

void listen_socket(int socket_file_descriptor) {
    if(listen(socket_file_descriptor, CONNECTIONS_WAITING) == -1) {
        perror("server: listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");
}

void printf_buffer(const byte_buffer* bbufer) {
   printf("Received %zu bytes: ", bbufer->len);
        for(size_t i = 0; i < bbufer->len; i++) {
            if(bbufer->data[i] == '\r') {
                printf("\\r");
            } else if (bbufer->data[i] == '\n') {
                printf("\\n");
            } else {
                printf("%c", bbufer->data[i]);
            }
        }

        printf("\n");
}

void* handle_client(void* args) {
    // reverse the cast: void* -> intptr_t -> int, to get back the file descriptor
    int new_file_descriptor= (int)(intptr_t) args;
    ssize_t recv_bytes = 0;
    byte_buffer bbufer;
    parsed_cmd* cmd = NULL;
    char buffer[1024];

    byte_buffer_init(&bbufer, 0);

    while((recv_bytes = recv(new_file_descriptor, buffer, sizeof(buffer), 0)) > 0) {
        if(cmd) free_parsed_cmd(cmd);
        if(!byte_buffer_append(&bbufer, buffer, recv_bytes)) {
            break;
        }

        printf_buffer(&bbufer);
 
        cmd = parse_cmd(bbufer.data, bbufer.len);

        if(cmd->status == PARSE_ERROR) {
            perror("server: error parsing cmd");
            break;
        } else if (cmd->status == PARSE_INCOMPLETE) {
            continue;
        } else {
            //THERE IS A CHANCE THAT CMD_RESULT MIGHT BE NULL. IF THAT IS the case we need to deal with it either in the encode or somewhere.
            cmd_result cmd_result = execute_cmd(cmd->cmd_name, cmd->buffer, cmd->arg_lengths, cmd->argc);
            
            // then encode the info we want to send the user
            // then send a response with the enconded data back to the user.
            // this is to be done after we have the resp protocol and the hashmap implemented.
            break;
        }
    }

    byte_buffer_destroy(&bbufer);
    if(cmd) free_parsed_cmd(cmd);
    close(new_file_descriptor);
    printf("server: client disconnected, waiting for new connections...\n");

    return NULL;
}

void accept_new_connections(int socket_file_descriptor) {
    socklen_t sockaddr_in_size;
    struct sockaddr_storage their_addr;
    int new_file_descriptor;
    
    while(1) {
        sockaddr_in_size = sizeof their_addr;
        new_file_descriptor = accept(socket_file_descriptor, (struct sockaddr*) &their_addr, &sockaddr_in_size);

        if(new_file_descriptor == -1) {
            perror("server: accept");
            continue;
        }

        pthread_t th;
        // pass the int file descriptor directly as the thread argument (no malloc needed
        // for a single small value); cast through intptr_t first since it matches void*'s
        // size on this platform, avoiding a compiler warning for int -> pointer directly
        if(pthread_create(&th, NULL, &handle_client, (void*)(intptr_t) new_file_descriptor) != 0) {
            perror("failed to create thread");
            close(new_file_descriptor);
            continue;
        }

        pthread_detach(th);

    }
}

int main(void) {
    struct addrinfo hints;
    struct addrinfo* server_info;
    int get_addr_info_status;
    int listening_socket_fd;

    resolve_server_address(&hints, &get_addr_info_status, &server_info);
    create_listening_socket(server_info, &listening_socket_fd);
    listen_socket(listening_socket_fd);
    accept_new_connections(listening_socket_fd);

    close(listening_socket_fd);
    return EXIT_SUCCESS;
}