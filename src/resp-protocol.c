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
            .arg_lengths = 0,
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
            if(checking_number_args && buffer[i] != '\n' && buffer[i] != '\r') {
                byte_buffer_append(&temp_buffer, &buffer[i], 1);
            } else if (checking_number_args && buffer[i] == '\n') {
                for(size_t j = 0; j < temp_buffer.len; j++) {
                    char* endptr;
                    unsigned long num = strtoul(temp_buffer.data, &endptr, 10);
                    cmd->argc = num;
                    byte_buffer_destroy(&temp_buffer);
                }
                checking_number_args = false;
            }
        }

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