#include "resp-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "byte-buffer.h"
typedef enum {
    EXPECTING_LENGTH,
    EXPECTING_CONTENT,
} arg_parse_state;

typedef enum {
    PROGRESS_MADE,   // processed something, but nothing else to do for this character
    ARG_COMPLETE,    // finished an argument/command, ready to move forward
    PARSE_FAILED,    // allocation failure or similar internal error
} arg_parse_result;

static bool set_parse_type(const char* buffer, const size_t index, bool* checking_number_args, bool* checking_arg) {
    // if we are getting the number of argc
    if(buffer[index] == '*') {
        *checking_number_args = true;
        return true;
    }

    // if we are checking an argument
    if(buffer[index] == '$') {
        *checking_arg = true;
        return true;
    }

    return false;
}

static bool get_number_of_args(bool* checking_number_args, const char* buffer, const size_t index, byte_buffer* temp_buffer, parsed_cmd* cmd) {
    if(*checking_number_args) {
        if(buffer[index] != '\n' && buffer[index] != '\r') {
            byte_buffer_append(temp_buffer, &buffer[index], 1);
        } else if (buffer[index] == '\n') {
            char terminator = '\0';
            byte_buffer_append(temp_buffer, &terminator, 1);
            char* endptr;
            unsigned long num = strtoul(temp_buffer->data, &endptr, 10);
            cmd->argc = num - 1; 

            cmd->buffer = malloc((num) * sizeof(char*));
            if(!cmd->buffer){
                free_parsed_cmd(cmd);
                byte_buffer_destroy(temp_buffer);
                return false;
            }

            cmd->arg_lengths = malloc((num) * sizeof(unsigned long));
            if(!cmd->arg_lengths) {
                free_parsed_cmd(cmd);
                byte_buffer_destroy(temp_buffer);
                return false;
            }

            *checking_number_args = false;
            byte_buffer_reset(temp_buffer);
        }
    }

    return true;
}

static arg_parse_result parse_argument(bool* checking_arg, const char* buffer, const size_t index, size_t* current_arg, parsed_cmd* cmd, const size_t buffer_len, byte_buffer* temp_buffer, arg_parse_state* current_state) {
    if(*checking_arg) {
        if(buffer[index] == '\n') { 
            // if its the last arg, its the end of the resp parsing so save the bytes consumed
            if(*current_arg == cmd->argc) {
                cmd->bytes_consumed = buffer_len;
            }

            // if we have a cmd to save, save
            // Should only pass here once
            if(*current_arg == 0) {
                if(*current_state == EXPECTING_LENGTH) {
                    *current_state = EXPECTING_CONTENT;
                    byte_buffer_reset(temp_buffer);
                    return ARG_COMPLETE;
                }

                cmd->cmd_name = malloc(temp_buffer->len + 1);
                if(!cmd->cmd_name) {
                    free_parsed_cmd(cmd);
                    byte_buffer_destroy(temp_buffer);
                    return PARSE_FAILED;
                }

                memcpy(cmd->cmd_name, temp_buffer->data, temp_buffer->len);
                cmd->cmd_name[temp_buffer->len] = '\0';

                *current_state = EXPECTING_LENGTH;
                *current_arg += 1;
                byte_buffer_reset(temp_buffer);
                return ARG_COMPLETE;
            }

            // means we have the bytes for the current arg we are parsing
            if(*current_arg > 0 && *current_state == EXPECTING_LENGTH) {
                char terminator = '\0';
                byte_buffer_append(temp_buffer, &terminator, 1);
                char* endptr;
                unsigned long current_num = strtoul(temp_buffer->data, &endptr, 10);
                cmd->arg_lengths[*current_arg - 1] = current_num;

                *current_state = EXPECTING_CONTENT;
                byte_buffer_reset(temp_buffer);
                return ARG_COMPLETE;
            }

            // means we have an arg that is not a byte and that is not a cmd to save
            if (*current_arg > 0 && *current_state == EXPECTING_CONTENT) { 
                cmd->buffer[*current_arg - 1] = malloc(temp_buffer->len);
                if(!cmd->buffer[*current_arg - 1]) { 
                    free_parsed_cmd(cmd);
                    byte_buffer_destroy(temp_buffer);
                    return PARSE_FAILED;
                }
                memcpy(cmd->buffer[*current_arg - 1], temp_buffer->data, temp_buffer->len);

                *current_state = EXPECTING_LENGTH;
                *current_arg += 1;
                byte_buffer_reset(temp_buffer);
                return ARG_COMPLETE;
            }
        }

        if(buffer[index] != '\n' && buffer[index] != '\r') {
            byte_buffer_append(temp_buffer, &buffer[index], 1);
        }
    }

    return PROGRESS_MADE;
}

static void print_buffer(parsed_cmd* cmd) {
    for(size_t k = 0; k < cmd->argc; k++) {
        for(size_t t = 0; t < cmd->arg_lengths[k]; t++) {
            printf("%c", cmd->buffer[k][t]);
        } 
            printf(" ");
    }

    printf("\n");
}

parsed_cmd* parse_cmd(char* buffer, size_t buffer_len) {
    if(buffer) {
        parsed_cmd* cmd = malloc(sizeof(parsed_cmd));
        if(!cmd) return NULL;

        *cmd = (parsed_cmd) {
            .status = PARSE_INCOMPLETE
        };

        size_t i;
        bool checking_number_args = false;
        bool checking_arg = false;
        byte_buffer temp_buffer;
        byte_buffer_init(&temp_buffer, 0);
        size_t current_arg = 0;
        arg_parse_state current_state = EXPECTING_LENGTH;

        for(i = 0; i < buffer_len; i++) {
            if(set_parse_type(buffer, i, &checking_number_args, &checking_arg)) continue;
            if(!get_number_of_args(&checking_number_args, buffer, i, &temp_buffer, cmd)) return NULL;
            arg_parse_result parse_result = parse_argument(&checking_arg, buffer, i, &current_arg, cmd, buffer_len, &temp_buffer, &current_state);
            if(parse_result == ARG_COMPLETE) {
                continue;
            } else if (parse_result == PARSE_FAILED) {
                return NULL;
            }
        }

        if(current_arg == cmd->argc + 1) {
            cmd->status = PARSE_COMPLETE;
        }

        print_buffer(cmd);

        byte_buffer_destroy(&temp_buffer);
        return cmd;
    }

    return NULL;
}

void free_parsed_cmd(parsed_cmd* cmd) {
    if(cmd) {
        if(cmd->buffer) {
            for(size_t i = 0; i < cmd->argc; i++) {
                if(cmd->buffer[i]) free(cmd->buffer[i]);
            }

            free(cmd->buffer);
        }

        if(cmd->arg_lengths) {
            free(cmd->arg_lengths);
        }

        if(cmd->cmd_name) {
            free(cmd->cmd_name);
        }

        *cmd = (parsed_cmd) { };
        free(cmd);
    } 
}

// encoded_resp* encode_resp(cmd_result* cmd) {

// }

// void free_encoded_resp(encoded_resp* resp) {

// }