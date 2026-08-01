#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

typedef enum {
    SIMPLE_STRING,
    BULK_STRING,
    SIMPLE_ERROR,
    SYSTEM_ERROR,
} result_status;

typedef struct {
    result_status status;
    char* result;
    size_t result_len;
} cmd_result;

cmd_result execute_cmd(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc);
void free_cmd(cmd_result cmd);

#endif