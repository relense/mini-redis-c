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

cmd_result execute_cmd(char* cmd_name, char** buffer, size_t* arg_lengths, size_t argc) {
    cmd_result result;
    
    if(strcasecmp(cmd_name, "GET") == 0) {
        char* get_result = storage_get(buffer[0]);

       result = (cmd_result) {
            .result = get_result,
            .result_len = strlen(get_result),
            .status = BULK_STRING
        };

        return result;
    } else if(strcasecmp(cmd_name, "SET") == 0) {
    
    } else if(strcasecmp(cmd_name, "DEL") == 0) {
        bool del_result = storage_del(buffer[0]);
        char* message = '+OK';

        if(del_result) {
            result = (cmd_result) {
                .result = message,
                .result_len = strlen(message),
                .status = SIMPLE_STRING
            };
    
            return result;
        } else {
            message = 'AINDA NAO SEI';
            result = (cmd_result) {
                .result = message, //VERIFICAR DEPOIS COMO TRATO QUANDO não remove ou não existe para remover,
                .result_len = strlen(message),
                .status = BULK_STRING
            };
    
            return result;
        }
    } else if(strcasecmp(cmd_name, "PING") == 0) {
        char* message = 'PONG';

        result = (cmd_result) {
            .result = message,
            .result_len = strlen(message),
            .status = SIMPLE_STRING
        };

        return result;
    } else {
        char* message = '-Error unknown command' + cmd_name;
        result = (cmd_result) {
            .result = message,
            .result_len = strlen(message),
            .status = SIMPLE_ERROR
        };

        return result;
    }
}
