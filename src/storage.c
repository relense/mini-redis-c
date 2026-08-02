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
    if(!hash_map_put(&map, key, value, value_len)) {
        *system_error = true;
    } else {
        *system_error = false;
    }
}

storage_result storage_get(const char* key) {
    entry_result found_entry;

    found_entry = hash_map_get(&map, key);
    if(!found_entry.value) {
        return (storage_result) {
            .key_exists = false,
            .value = NULL,
            .value_len = 0
        };
    } else {
        return (storage_result) {
            .key_exists = true,
            .value = found_entry.value,
            .value_len = found_entry.value_len
        };
    }
}

void storage_del(const char* key) {
    hash_map_remove(&map, key);
}