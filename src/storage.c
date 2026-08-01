#include "storage.h"
#include <stdbool.h>

#include "hash-map.h"

static hash_map map;

void storage_init(void) {
    hash_map_init(&map, 10);
}

void storage_destroy(void) {
    hash_map_destroy(&map);
}

void storage_set(const char* key, const char* value, size_t value_len, bool* system_error) {

}

storage_result storage_get(const char* key) {

}

void storage_del(const char* key) {

}