#include "commands.h"
#include "storage.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    GET,
    SET,
    DEL,
    PING
} available_cmds;

typedef enum {
    TOO_MANY_ARGS,
    CMD_UNKNOWN,
} error_types;

static char* to_null_terminated(char* value, size_t len) {
    char* new_value = malloc(len + 1);
    if(!new_value) return NULL;

    memcpy(new_value, value, len);
    new_value[len] = '\0';

    return new_value;
}

static cmd_result build_error_response(char* cmd_name, error_types error_type) {
    cmd_result result;
    char message[256];
    
    if(error_type == CMD_UNKNOWN) {
        snprintf(message, sizeof(message), "%s: unknown command", cmd_name);
    } else if (error_type == TOO_MANY_ARGS) {
        snprintf(message, sizeof(message), "%s: too many args", cmd_name);
    }

    result = (cmd_result) {
        .result = strdup(message),
        .result_len = strlen(message),
        .status = SIMPLE_ERROR
    };

    return result;
}

static cmd_result build_simple_string(char* message) {
    cmd_result result;

    result = (cmd_result) {
        .result = strdup(message),
        .result_len = strlen(message),
        .status = SIMPLE_STRING
    };

    return result;
}

static cmd_result build_system_error_response() {
    cmd_result result;

    result = (cmd_result) {
        .result = strdup(""),
        .result_len = 0,
        .status = SYSTEM_ERROR
    };

    return result;
}

static cmd_result execute_get(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(argc > 1) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    }

    cmd_result result;
    char* key = to_null_terminated(buffer[0], arg_lengths[0]);
    char* get_result = storage_get(key);

    if(!get_result) {
        result = (cmd_result) {
            .result = strdup(""),
            .result_len = 0,
            .status = BULK_STRING
        };
    } else {
        result = (cmd_result) {
            .result = strdup(get_result),
            .result_len = strlen(get_result),
            .status = BULK_STRING
        };
    }

    free(key);
    return result;
}

static cmd_result execute_set(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
     if(argc > 2) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    }

    bool system_error = false;
    char* key = to_null_terminated(buffer[0], arg_lengths[0]);
    char* value = to_null_terminated(buffer[1], arg_lengths[1]);

    storage_set(key, value, &system_error);

    free(key);
    free(value);

    if(system_error) {
        return build_system_error_response();
    }

    char* message = "OK";
    return build_simple_string(message);
}

static cmd_result execute_del(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(argc > 1) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    }

    char* key = to_null_terminated(buffer[0], arg_lengths[0]);
    storage_del(key);

    free(key);
    char* message = "OK";

    return build_simple_string(message);
}

static cmd_result execute_pong(char* cmd_name, size_t argc) {
    if(argc > 0) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    }

    char * message = "PONG";

    return build_simple_string(message);
}

cmd_result execute_cmd(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(strcasecmp(cmd_name, "GET") == 0) {
       return execute_get(cmd_name, buffer, arg_lengths, argc);
    } else if(strcasecmp(cmd_name, "SET") == 0) {
       return execute_set(cmd_name, buffer, arg_lengths, argc);
    } else if(strcasecmp(cmd_name, "DEL") == 0) {
        return execute_del(cmd_name, buffer, arg_lengths, argc);
    } else if(strcasecmp(cmd_name, "PING") == 0) {
        return execute_pong(cmd_name, argc);
    } else {
        return build_error_response(cmd_name, CMD_UNKNOWN);
    }
}

void free_cmd(cmd_result cmd) {
    free(cmd.result);
}