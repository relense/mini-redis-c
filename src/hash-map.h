#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct entry entry;

struct entry {
    char* key;
    char* value;
    entry* next;
};

typedef struct {
    size_t len;
    size_t cap;
    entry** buckets;
} hash_map;

hash_map* hash_map_init(hash_map* map, size_t cap);
void hash_map_destroy(hash_map* map);

hash_map* hash_map_put(hash_map* map, char* key, char* value);
bool hash_map_remove(hash_map* map, char* key);
char* hash_map_get(hash_map* map, char* key);

#endif