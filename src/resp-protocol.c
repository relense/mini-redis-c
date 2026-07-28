#include "resp-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "byte-buffer.h"

parsed_cmd* parse_cmd(char* buffer, size_t buffer_len) {
    if(buffer) {
        parsed_cmd* cmd = malloc(sizeof(parsed_cmd));
        if(!cmd) return NULL;

        *cmd = (parsed_cmd) {
            .arg_lengths = NULL,
            .argc = 0,
            .buffer = NULL,
            .bytes_consumed = 0,
            .cmd_name = NULL,
            .status = PARSE_INCOMPLETE
        };

        size_t i;
        bool checking_number_args = false;
        byte_buffer temp_buffer;
        byte_buffer_init(&temp_buffer, 0);
        bool checking_arg = false;
        size_t current_arg = 0;
        size_t new_line_count = 0;

        //loop to find number of args
        for(i = 0; i < buffer_len; i++) {
            if(buffer[i] == '*') {
                checking_number_args = true;
                continue;
            }

            if(buffer[i] == '$') {
                checking_arg = true;
                continue;
            }

            //getting the number of arguments
            if(checking_number_args) {
                if(buffer[i] != '\n' && buffer[i] != '\r') {
                    byte_buffer_append(&temp_buffer, &buffer[i], 1);
                } else if (buffer[i] == '\n') {
                    for(size_t j = 0; j < temp_buffer.len; j++) {
                        char* endptr;
                        unsigned long num = strtoul(temp_buffer.data, &endptr, 10);
                        cmd->argc = num;
                        byte_buffer_reset(&temp_buffer);
                    }
                    checking_number_args = false;
                }
            }

            if(checking_arg) {
                if(buffer[i] == '\n') { 
                    // if its the last arg, its the end of the resp parsing so save the bytes consumed
                    if(current_arg == cmd->argc - 1) {
                        cmd->bytes_consumed = temp_buffer.len;
                    }

                    //means we have a cmd to save
                    if(current_arg == 0 && new_line_count > 0) {
                        for(size_t j = 0; j < temp_buffer.len; j++) {
                            cmd->cmd_name[j] = temp_buffer.data[j];
                        }    
                    }

                    // means we have an arg that is not a byte and that is not a cmd to save
                    if (current_arg > 0 && new_line_count > 0) { 
                         for(size_t j = 0; j < temp_buffer.len; j++) {
                            cmd->buffer[j] = &temp_buffer.data[j];
                        }
                    }

                    // means we have the bytes for the current arg we are parsing
                    if(new_line_count == 0) { 
                            char* endptr;
                            unsigned long num = strtoul(temp_buffer.data, &endptr, 10);
                            cmd->arg_lengths[current_arg] = num;
                        new_line_count++;
                    }

                    if(new_line_count > 0) { //means it finished the arg parsing and we reset the new line count and set wich argument we are parsing to the next
                        new_line_count = 0;
                        current_arg++;
                    }

                    byte_buffer_reset(&temp_buffer);
                }

                if(buffer[i] != '\n' && buffer[i] != '\r') {
                    byte_buffer_append(&temp_buffer, &buffer[i], 1);
                }
            }
        }

        for(size_t k = 0; k < cmd->argc; k++) {
            for(size_t t = 0; t < cmd->arg_lengths[k]; t++) {
                if(cmd->buffer[k][t] == '\r') {
                    printf("\\r");
                } else if (cmd->buffer[k][t] == '\n') {
                    printf("\\n");
                } else {
                    printf("%c", cmd->buffer[k][t]);
                }    
            } 
        }

        printf("\n");

        return cmd;
    }

    return NULL;
}

void free_parsed_cmd(parsed_cmd* cmd) {
    if(cmd) {
        free(cmd);
    }
}

// encoded_resp* encode_resp(cmd_result* cmd) {

// }

// void free_encoded_resp(encoded_resp* resp) {

// }