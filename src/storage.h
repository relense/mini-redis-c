#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

typedef struct {
    char* value;
    size_t len;
} storage_result;

void storage_set(const char* key, const char* value, size_t value_len, bool* system_error);
storage_result storage_get(const char* key);
void storage_del(const char* key);

#endif