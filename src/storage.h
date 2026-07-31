#ifndef STORAGE_H
#define STORAGE_H

void storage_set(const char* key, const char* value);
char* storage_get(const char* key);
bool storage_del(const char* key);

#endif