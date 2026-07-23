#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CONNECTIONS_WAITING 10

void set_addr_info(struct addrinfo* hints, int* get_addr_info_status, struct addrinfo** server_info) {
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;

    if((*get_addr_info_status = getaddrinfo(NULL, "8080", hints, server_info)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(*get_addr_info_status));
        exit(1);
    }
}

void get_available_connection(struct addrinfo* server_info, int* socket_file_descriptor) {
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

void* handle_client(void* args) {
    // reverse the cast: void* -> intptr_t -> int, to get back the file descriptor
    int new_file_descriptor= (int)(intptr_t) args;
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
    int socket_file_descriptor;

    set_addr_info(&hints, &get_addr_info_status, &server_info);
    get_available_connection(server_info, &socket_file_descriptor);
    

    if(listen(socket_file_descriptor, CONNECTIONS_WAITING) == -1) {
        perror("server: listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    accept_new_connections(socket_file_descriptor);

    close(socket_file_descriptor);
    return EXIT_SUCCESS;
}