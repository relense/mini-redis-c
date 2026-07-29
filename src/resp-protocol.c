#include "resp-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "byte-buffer.h"
typedef enum {
    EXPECTING_LENGTH,
    EXPECTING_CONTENT,
} arg_parse_state;
typedef enum {
    STEP_PROGRESS,      // processed something, nothing else to do for this character
    STEP_ARG_COMPLETE,   // finished an argument/command, ready to move forward
    STEP_ALLOC_FAILED,   // allocation failure or similar internal error
    STEP_SYNTAX_ERROR,   // RESP protocol syntax was violated
} arg_parse_result;

typedef enum {
    READ_HEADER,
    READ_ARG_LEN,
    READ_ARG_DATA
} parse_cmd_state;

static arg_parse_result get_number_of_args(bool* checking_number_args, const char* buffer, const size_t index, byte_buffer* temp_buffer, parsed_cmd* cmd) {
    if(buffer[index] != '\n' && buffer[index] != '\r') {
        if(!isdigit(buffer[index])) {
            cmd->status = PARSE_ERROR;
            byte_buffer_destroy(temp_buffer);
            return STEP_SYNTAX_ERROR;
        };

        byte_buffer_append(temp_buffer, &buffer[index], 1);
    } else if (buffer[index] == '\n') {
        char terminator = '\0';
        byte_buffer_append(temp_buffer, &terminator, 1);
        char* endptr;
        unsigned long num = strtoul(temp_buffer->data, &endptr, 10);

        if(num == 0) {
            cmd->status = PARSE_ERROR;
            byte_buffer_destroy(temp_buffer);
            return STEP_SYNTAX_ERROR;
        }

        cmd->argc = num - 1; 

        cmd->buffer = malloc((cmd->argc) * sizeof(char*));
        if(!cmd->buffer){
            free_parsed_cmd(cmd);
            byte_buffer_destroy(temp_buffer);
            return STEP_ALLOC_FAILED;
        }

        cmd->arg_lengths = malloc((cmd->argc) * sizeof(unsigned long));
        if(!cmd->arg_lengths) {
            free_parsed_cmd(cmd);
            byte_buffer_destroy(temp_buffer);
            return STEP_ALLOC_FAILED;
        }

        *checking_number_args = false;
        byte_buffer_reset(temp_buffer);
    }

    return STEP_PROGRESS;
}

static arg_parse_result parse_argument(const char* buffer, const size_t index, size_t* current_arg, parsed_cmd* cmd, const size_t buffer_len, byte_buffer* temp_buffer, arg_parse_state* current_state) {
    if(buffer[index] == '\n') { 
        // if its the last arg, its the end of the resp parsing so save the bytes consumed
        if(*current_arg == cmd->argc && *current_state == EXPECTING_CONTENT) {
            cmd->bytes_consumed = buffer_len;
        }

        // if we have a cmd to save, save
        // Should only pass here once
        if(*current_arg == 0) {
            if(*current_state == EXPECTING_LENGTH) {
                *current_state = EXPECTING_CONTENT;
                byte_buffer_reset(temp_buffer);
                return STEP_ARG_COMPLETE;
            }

            cmd->cmd_name = malloc(temp_buffer->len + 1);
            if(!cmd->cmd_name) {
                free_parsed_cmd(cmd);
                byte_buffer_destroy(temp_buffer);
                return STEP_ALLOC_FAILED;
            }

            memcpy(cmd->cmd_name, temp_buffer->data, temp_buffer->len);
            cmd->cmd_name[temp_buffer->len] = '\0';

            *current_state = EXPECTING_LENGTH;
            *current_arg += 1;
            byte_buffer_reset(temp_buffer);
            return STEP_ARG_COMPLETE;
        }

        // means we have the bytes for the current arg we are parsing
        if(*current_arg > 0 && *current_state == EXPECTING_LENGTH) {
            char terminator = '\0';
            byte_buffer_append(temp_buffer, &terminator, 1);
            char* endptr;
            unsigned long current_num = strtoul(temp_buffer->data, &endptr, 10);

            if(current_num == 0) {
                cmd->status = PARSE_ERROR;
                byte_buffer_destroy(temp_buffer);
                return STEP_SYNTAX_ERROR;
            }

            cmd->arg_lengths[*current_arg - 1] = current_num;

            *current_state = EXPECTING_CONTENT;
            byte_buffer_reset(temp_buffer);
            return STEP_ARG_COMPLETE;
        }

        // means we have an arg that is not a byte and that is not a cmd to save
        // the current_arg - 1 is because current_arg = 0 is the arg for cmd but in the buffer we want the other args so we must start at 0.
        if (*current_arg > 0 && *current_state == EXPECTING_CONTENT) { 
            cmd->buffer[*current_arg - 1] = malloc(temp_buffer->len);
            if(!cmd->buffer[*current_arg - 1]) { 
                free_parsed_cmd(cmd);
                byte_buffer_destroy(temp_buffer);
                return STEP_ALLOC_FAILED;
            }
            memcpy(cmd->buffer[*current_arg - 1], temp_buffer->data, temp_buffer->len);

            *current_state = EXPECTING_LENGTH;
            *current_arg += 1;
            byte_buffer_reset(temp_buffer);
            return STEP_ARG_COMPLETE;
        }
    }

    if(buffer[index] != '\n' && buffer[index] != '\r') {
        if(*current_state == EXPECTING_LENGTH && !isdigit(buffer[index])) {
            cmd->status = PARSE_ERROR;
            byte_buffer_destroy(temp_buffer);
            return STEP_SYNTAX_ERROR;
        };

        byte_buffer_append(temp_buffer, &buffer[index], 1);
    }

    return STEP_PROGRESS;
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

        if(buffer[0] != '*') {
            cmd->status = PARSE_ERROR;
            return cmd;
        }

        size_t i;
        bool checking_number_args = true;
        byte_buffer temp_buffer;
        byte_buffer_init(&temp_buffer, 0);
        size_t current_arg = 0;
        arg_parse_state current_state = EXPECTING_LENGTH;

        for(i = 0; i < buffer_len; i++) {
            if(checking_number_args) {
                arg_parse_result parse_number_args_result = get_number_of_args(&checking_number_args, buffer, i, &temp_buffer, cmd);
                if(parse_number_args_result == STEP_ALLOC_FAILED) {
                    return NULL;
                } else if (parse_number_args_result == STEP_SYNTAX_ERROR) {
                    return cmd;
                }
            }

            if(!checking_number_args) {
                arg_parse_result parse_result = parse_argument(buffer, i, &current_arg, cmd, buffer_len, &temp_buffer, &current_state);
                if(parse_result == STEP_ARG_COMPLETE) {
                    continue;
                } else if (parse_result == STEP_ALLOC_FAILED) {
                    return NULL;
                } else if (parse_result == STEP_SYNTAX_ERROR) {
                    return cmd;
                }
            }
        }

        if(current_arg == cmd->argc + 1) {
            cmd->status = PARSE_COMPLETE;
            print_buffer(cmd);
        }

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