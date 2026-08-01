#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

void storage_set(const char* key, const char* value, bool* system_error);
char* storage_get(const char* key);
void storage_del(const char* key);

#endif