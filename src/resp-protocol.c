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
    STEP_PROGRESS,
    STEP_ARG_LEN_COMPLETE,    // finished reading a length ($M or *N), \n was the real terminator
    STEP_ARG_CONTENT_COMPLETE, // finished reading binary content, \r\n that follows is residual
    STEP_ALLOC_FAILED,
    STEP_SYNTAX_ERROR,
} arg_parse_result;

typedef enum {
    READ_HEADER,
    READ_ARG_LEN,
    READ_ARG_DATA
} parse_cmd_state;

#define MAX_ARGS 50
#define MAX_ARGS_LENGTH (1024 * 1024) //equivalent in bytes to 1MB

// Parses the *N header (array length) from the buffer at the current index.
// Allocates cmd->buffer and cmd->arg_lengths once argc is known. Rejects
// zero, non-numeric, or over-limit counts (MAX_ARGS) as syntax errors.
static arg_parse_result get_number_of_args(bool* checking_number_args, const char* buffer, const size_t index, byte_buffer* temp_buffer, parsed_cmd* cmd) {
    if(*checking_number_args) {
        if(buffer[index] != '\n' && buffer[index] != '\r') {
            if(!isdigit(buffer[index])) {
                byte_buffer_destroy(temp_buffer);
                return STEP_SYNTAX_ERROR;
            };

            byte_buffer_append(temp_buffer, &buffer[index], 1);
        } else if (buffer[index] == '\n') {
            char terminator = '\0';
            byte_buffer_append(temp_buffer, &terminator, 1);
            char* endptr;
            unsigned long num = strtoul(temp_buffer->data, &endptr, 10);

            if(num == 0 || endptr == temp_buffer->data || num > MAX_ARGS) {
                byte_buffer_destroy(temp_buffer);
                return STEP_SYNTAX_ERROR;
            }

            cmd->argc = num - 1; 

            cmd->buffer = calloc(cmd->argc, sizeof(char*));
            if(!cmd->buffer){
                byte_buffer_destroy(temp_buffer);
                return STEP_ALLOC_FAILED;
            }

            cmd->arg_lengths = calloc(cmd->argc, sizeof(unsigned long));
            if(!cmd->arg_lengths) {
                byte_buffer_destroy(temp_buffer);
                return STEP_ALLOC_FAILED;
            }

            *checking_number_args = false;
            byte_buffer_reset(temp_buffer);
        }
    }

    return STEP_PROGRESS;
}

// Parses a single argument's $M length header and its content, one byte
// at a time. Tracks state via current_state (EXPECTING_LENGTH vs
// EXPECTING_CONTENT) and temp_byte_count. Handles arg 0 (the command name)
// as a special case since its length is not tracked in arg_lengths.
static arg_parse_result parse_argument(bool* checking_arg, const char* buffer, const size_t index, size_t* current_arg, parsed_cmd* cmd, const size_t buffer_len, byte_buffer* temp_buffer, arg_parse_state* current_state, size_t* temp_byte_count) {
    if(*checking_arg) {
        // if its the last arg, its the end of the resp parsing so save the bytes consumed
        if(*current_arg == cmd->argc && *current_state == EXPECTING_CONTENT) {
            cmd->bytes_consumed = buffer_len;
        }

        if(buffer[index] == '\n') { 
            // if we have a cmd to save, save
            // Should only pass here once
            if(*current_arg == 0) {
                //We don't care about the lenght for arg 0 which is cmd, so skip so we can get the cmd string
                if(*current_state == EXPECTING_LENGTH) {
                    *current_state = EXPECTING_CONTENT;
                    byte_buffer_reset(temp_buffer);
                    return STEP_ARG_LEN_COMPLETE;
                }

                cmd->cmd_name = malloc(temp_buffer->len + 1);
                if(!cmd->cmd_name) {
                    byte_buffer_destroy(temp_buffer);
                    return STEP_ALLOC_FAILED;
                }

                memcpy(cmd->cmd_name, temp_buffer->data, temp_buffer->len);
                cmd->cmd_name[temp_buffer->len] = '\0';

                *current_state = EXPECTING_LENGTH;
                *current_arg += 1;
                byte_buffer_reset(temp_buffer);
                return STEP_ARG_LEN_COMPLETE;
            }

            // means we have the bytes for the current arg we are parsing
            if(*current_arg > 0 && *current_state == EXPECTING_LENGTH) {
                char terminator = '\0';
                byte_buffer_append(temp_buffer, &terminator, 1);
                char* endptr;
                unsigned long current_num = strtoul(temp_buffer->data, &endptr, 10);

                if(endptr == temp_buffer->data || current_num > MAX_ARGS_LENGTH) {
                    byte_buffer_destroy(temp_buffer);
                    return STEP_SYNTAX_ERROR;
                }

                cmd->arg_lengths[*current_arg - 1] = current_num;

                *current_state = EXPECTING_CONTENT;
                byte_buffer_reset(temp_buffer);
                return STEP_ARG_LEN_COMPLETE;
            }
        }

        // means we have an arg that is not a byte and that is not a cmd to save
        // the current_arg - 1 is because current_arg = 0 is the arg for cmd but in the buffer we want the other args so we must start at 0.
        if (*current_arg > 0 && *current_state == EXPECTING_CONTENT && *temp_byte_count == cmd->arg_lengths[*current_arg - 1]) { 
            size_t alloc_size = temp_buffer->len > 0 ? temp_buffer->len : 1;
            cmd->buffer[*current_arg - 1] = malloc(alloc_size);
            if(!cmd->buffer[*current_arg - 1]) { 
                byte_buffer_destroy(temp_buffer);
                return STEP_ALLOC_FAILED;
            }
            memcpy(cmd->buffer[*current_arg - 1], temp_buffer->data, temp_buffer->len);

            *current_state = EXPECTING_LENGTH;
            *current_arg += 1;
            *temp_byte_count = 0;
            byte_buffer_reset(temp_buffer);
            return STEP_ARG_CONTENT_COMPLETE;
        }

        if(*current_arg > 0 && *current_state == EXPECTING_LENGTH && buffer[index] != '\n' && buffer[index] != '\r') {
            if(!isdigit(buffer[index])) {
                byte_buffer_destroy(temp_buffer);
                return STEP_SYNTAX_ERROR;
            };

            byte_buffer_append(temp_buffer, &buffer[index], 1);
        } else if (*current_arg > 0 && *current_state == EXPECTING_CONTENT) {
            if(*temp_byte_count < cmd->arg_lengths[*current_arg - 1]) { // if our byte count is smaller then the arg length for this arg add the character
                *temp_byte_count += 1;
                byte_buffer_append(temp_buffer, &buffer[index], 1);
            }
        } else if (*current_arg == 0 && *current_state == EXPECTING_CONTENT && buffer[index] != '\n' && buffer[index] != '\r') {
            byte_buffer_append(temp_buffer, &buffer[index], 1);
        }
    }

    return STEP_PROGRESS;
}

// Entry point: parses a raw RESP buffer into a parsed_cmd. Validates the
// leading *, drives the byte-by-byte loop via get_number_of_args and
// parse_argument, and sets cmd->status to PARSE_COMPLETE or PARSE_ERROR
// once done. Returns NULL only if the initial cmd allocation itself fails.
parsed_cmd* parse_cmd(char* buffer, size_t buffer_len) {
    if(buffer) {
        parsed_cmd* cmd = malloc(sizeof(parsed_cmd));
        if(!cmd) return NULL;

        *cmd = (parsed_cmd) {
            .status = PARSE_INCOMPLETE
        };

        bool checking_number_args = false;
        bool checking_arg = false;
        size_t i = 0;

        if(buffer[0] != '*') {
            cmd->status = PARSE_ERROR;
            return cmd;
        } else {
            checking_number_args = true;
        }
        
        byte_buffer temp_buffer;
        byte_buffer_init(&temp_buffer, 0);
        size_t current_arg = 0;
        arg_parse_state current_state = EXPECTING_LENGTH;
        size_t temp_byte_count = 0;

        // lets go through the buffer to find the elements we need
        for(i = 1; i < buffer_len; i++) {
            if(buffer[i] == '$' && current_state != EXPECTING_CONTENT) {
                checking_arg = true;
                continue;
            }

            arg_parse_result parse_number_args_result = get_number_of_args(&checking_number_args, buffer, i, &temp_buffer, cmd);
            if(parse_number_args_result == STEP_ALLOC_FAILED || parse_number_args_result == STEP_SYNTAX_ERROR) {
                cmd->status = PARSE_ERROR;
                return cmd;
            }

            arg_parse_result parse_result = parse_argument(&checking_arg, buffer, i, &current_arg, cmd, buffer_len, &temp_buffer, &current_state, &temp_byte_count);
            if(parse_result == STEP_ARG_LEN_COMPLETE) {
                continue;
            } else if(parse_result == STEP_ARG_CONTENT_COMPLETE) {
                i++;
                continue;
            } else if (parse_result == STEP_ALLOC_FAILED || parse_result == STEP_SYNTAX_ERROR) {
                cmd->status = PARSE_ERROR;
                return cmd;
            }
        }

        if(current_arg == cmd->argc + 1) {
            cmd->status = PARSE_COMPLETE;
        }

        byte_buffer_destroy(&temp_buffer);
        return cmd;
    }

    return NULL;
}

// Frees a parsed_cmd and everything it owns: each argument buffer,
// arg_lengths, cmd_name, and the struct itself. Safe to call with any
// combination of fields still NULL (e.g. after an early parse error).
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

encoded_resp encode_resp(cmd_result* cmd) {
    if(cmd) {
        encoded_resp resp;

        if(cmd->status == SIMPLE_STRING) {
            size_t bytes = cmd->result_len + 4;
            char* value = malloc(bytes);
            if(!value) {
                 return (encoded_resp) {
                    .bytes_encoded = 0,
                    .resp =  NULL,
                };
            }

            snprintf(value, bytes, "+%s\r\n", cmd->result);

            resp.resp = value;
            resp.bytes_encoded = bytes - 1;
        } else if(cmd->status == SIMPLE_ERROR) {
            size_t bytes = cmd->result_len + 10;
            char* value = malloc(bytes);
            if(!value) {
                 return (encoded_resp) {
                    .bytes_encoded = 0,
                    .resp =  NULL,
                };
            }

            snprintf(value, bytes, "-Error %s\r\n", cmd->result);

            resp.resp = value;
            resp.bytes_encoded = bytes - 1;
        } else if(cmd->status == BULK_STRING) {
            int arg_len_bytes = snprintf(NULL, 0, "$%zu\r\n", cmd->result_len);
            size_t total_bytes = arg_len_bytes + cmd->result_len + 2;

            char* value = malloc(total_bytes);
            if(!value) {
                 return (encoded_resp) {
                    .bytes_encoded = 0,
                    .resp =  NULL,
                };
            }

            snprintf(value, total_bytes, "$%zu\r\n", cmd->result_len);
            memcpy(value + arg_len_bytes, cmd->result, cmd->result_len);
            value[total_bytes - 2] = '\r';
            value[total_bytes - 1] = '\n';

            resp.resp = value;
            resp.bytes_encoded = total_bytes;
        } else if(cmd->status == NULL_BULK_STRING) {
            size_t bytes = 5;
            char* value = malloc(bytes);
             if(!value) {
                 return (encoded_resp) {
                    .bytes_encoded = 0,
                    .resp =  NULL,
                };
            }

            memcpy(value, "$-1\r\n", bytes);

            resp.resp = value;
            resp.bytes_encoded = bytes;
        } else {
            return (encoded_resp) {
                .bytes_encoded = 0,
                .resp =  NULL,
            };
        }

        return resp;
    }

    return (encoded_resp) {
        .bytes_encoded = 0,
        .resp =  NULL,
    };
}

void free_encoded_resp(encoded_resp resp) {
    free(resp.resp);
}