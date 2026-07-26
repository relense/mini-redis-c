#include "resp-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

        for(size_t i = 0; i < buffer_len; i++) {
            if(!strcmp(&buffer[0],"*")) {
                continue;
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