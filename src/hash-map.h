#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct entry entry;

struct entry {
    char* key;
    char* value;
    size_t value_len;
    entry* next;
};

typedef struct {
    size_t len;
    size_t cap;
    entry** buckets;
    pthread_mutex_t hash_map_mutex;
} hash_map;

typedef struct {
    char* value;
    size_t value_len;
} entry_result;

hash_map* hash_map_init(hash_map* map, size_t cap);
void hash_map_destroy(hash_map* map);

hash_map* hash_map_put(hash_map* map, const char* key, const char* value, size_t value_len);
bool hash_map_remove(hash_map* map, const char* key);
entry_result hash_map_get(hash_map* map, const char* key);

#endif