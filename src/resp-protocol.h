#ifndef RESP_PROTOCOL_H
#define RESP_PROTOCOL_H

#include <sys/types.h>

typedef enum {
    PARSE_INCOMPLETE,
    PARSE_COMPLETE,
    PARSE_ERROR,
} parse_status;

typedef struct {
    parse_status status;
    char* cmd_name;
    char** buffer;
    int argc;
    size_t bytes_consumed;
} parsed_cmd;

typedef struct {
    char* resp;
    size_t bytes_encoded;
} encoded_resp;

//parse operations
parsed_cmd* parse_cmd(char* buffer, ssize_t buffer_len);
void free_parsed_cmd(parsed_cmd* cmd);

//encode_operations
encoded_resp* encode_resp(char* data);
void free_encoded_resp(encoded_resp* resp);

#endif


