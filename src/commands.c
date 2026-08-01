#include "commands.h"
#include "storage.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    TOO_MANY_ARGS,
    NOT_ENOUGH_ARGS,
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
    } else if (error_type == NOT_ENOUGH_ARGS) {
        snprintf(message, sizeof(message), "%s: not enough args", cmd_name);
    }

    char* copied_message = strdup(message);
    if(!copied_message) return build_system_error_response();

    result = (cmd_result) {
        .result = copied_message,
        .result_len = strlen(copied_message),
        .status = SIMPLE_ERROR
    };

    return result;
}

static cmd_result build_bulk_string(char* message, size_t len) {
    cmd_result result;

    char* copied_message = malloc(len > 0 ? len : 1);
    if(!copied_message) return build_system_error_response();
    memcpy(copied_message, message, len);

    result = (cmd_result) {
        .result = copied_message,
        .result_len = len,
        .status = BULK_STRING
    };

    return result;
}

static cmd_result build_null_bulk_string() {
    return (cmd_result) {
        .result = NULL,
        .result_len = 0,
        .status = NULL_BULK_STRING
    };
}

static cmd_result build_simple_string(char* message) {
    cmd_result result;

    char* copied_message = strdup(message);
    if(!copied_message) return build_system_error_response();

    result = (cmd_result) {
        .result = copied_message,
        .result_len = strlen(copied_message),
        .status = SIMPLE_STRING
    };

    return result;
}

static cmd_result build_system_error_response() {
    cmd_result result;
    char* message = strdup("");

    result = (cmd_result) {
        .result = message,
        .result_len = 0,
        .status = SYSTEM_ERROR
    };

    return result;
}

static cmd_result execute_get(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(argc > 1) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    } else if (argc < 1) {
        return build_error_response(cmd_name, NOT_ENOUGH_ARGS);
    } else {
        char* key = to_null_terminated(buffer[0], arg_lengths[0]);
        if(!key) {
            return build_system_error_response();
        }

        storage_result get_result = storage_get(key);
        free(key);

        if(!get_result.key_exists) {
             return build_null_bulk_string();
        }

        if(!get_result.value) {
            return build_bulk_string("", 0);
        } else {
            return build_bulk_string(get_result.value, get_result.len);
        }
    }
}

static cmd_result execute_set(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(argc > 2) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    } else if (argc < 2) {
        return build_error_response(cmd_name, NOT_ENOUGH_ARGS);
    } else {
        bool system_error = false;
        char* key = to_null_terminated(buffer[0], arg_lengths[0]);
        if(!key) {
            return build_system_error_response();
        }
        char* value = to_null_terminated(buffer[1], arg_lengths[1]);
        if(!value) {
            free(key);
            return build_system_error_response();
        }

        storage_set(key, value, arg_lengths[1], &system_error);

        free(key);
        free(value);

        if(system_error) {
            return build_system_error_response();
        }

        char* message = "OK";
        return build_simple_string(message);
    }
}

static cmd_result execute_del(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    if(argc > 1) {
        return build_error_response(cmd_name, TOO_MANY_ARGS);
    } else if (argc < 1) {
        return build_error_response(cmd_name, NOT_ENOUGH_ARGS);
    } else {
        char* key = to_null_terminated(buffer[0], arg_lengths[0]);
        if(!key) {
            return build_system_error_response();
        }
    
        storage_del(key);

        free(key);
        char* message = "OK";

        return build_simple_string(message);
    }
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