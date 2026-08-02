#include "hash-map.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

static size_t hash(char* string) {
    size_t result = 5381;
    size_t string_length = strlen(string);

    for(size_t i = 0; i < string_length; i++) {
        result = (result * 33) + string[i];
    }

    return result;
}

static hash_map* hash_map_insert(hash_map* map, entry* new_entry) {
    if(map && new_entry) {
        new_entry->next = NULL;
        size_t bucket_index = hash(new_entry->key) %map->cap;
        entry* current_bucket = map->buckets[bucket_index];

        if(!current_bucket) {
            map->buckets[bucket_index] = new_entry;
        } else {
            while(current_bucket->next != NULL) {
                current_bucket = current_bucket->next;
            }

            current_bucket->next = new_entry;
        }

        return map;
    }

    return NULL;
}

static hash_map* hash_map_resize(hash_map* map) {
    if(map) {
        entry** temp_entries = malloc(sizeof(entry*[map->len]));
        if(!temp_entries) return NULL;

        size_t visited_entries = 0;
        for(size_t i = 0; i < map->cap; i++) {
            entry* head = map->buckets[i];
            while(head != NULL) {
                temp_entries[visited_entries] = head;
                visited_entries++;
                head = head->next;
            }
        }

        size_t new_cap = map->cap * 2;
        entry** new_buckets = realloc(map->buckets, sizeof(entry*[new_cap]));

        if(!new_buckets) return NULL;

        map->buckets = new_buckets;
        map->cap = new_cap;

         for(size_t i = 0; i < map->cap; i++) {
            map->buckets[i] = NULL;
        }

        for(size_t j = 0; j < map->len; j++) {
            hash_map_insert(map, temp_entries[j]);
        }

        free(temp_entries);

        return map;
    }

    return NULL;
}

static entry* get_entry(hash_map* map, char* key) {
    if(map) {
        size_t bucket_index = hash(key) % map->cap;
        entry* current_entry = map->buckets[bucket_index];

        while(current_entry != NULL) {
            if(!strcmp(current_entry->key, key)) break;
            current_entry = current_entry->next;
        }

        if(current_entry == NULL) return NULL;

        return current_entry;
    }

    return NULL;
}

static void entry_destroy(entry* head) {
    if(head) {
        entry* current = head;

        while(current != NULL) {
            entry* next = current->next;
            free(current->key);
            free(current->value);
            free(current);

            current = next;
        };
    }
}

void hash_map_destroy(hash_map* map) {
    if(map) {
        pthread_mutex_lock(&map->hash_map_mutex);

        for(size_t i = 0; i < map->cap; i++) {
            entry_destroy(map->buckets[i]);
        }

        free(map->buckets);
        *map = (hash_map) {};

        pthread_mutex_unlock(&map->hash_map_mutex);
        pthread_mutex_destroy(&map->hash_map_mutex);
    }
}

hash_map* hash_map_init(hash_map* map, size_t cap) {
    if(map) {
        size_t actual_cap = (cap == 0) ? 16 : cap;

        *map = (hash_map) {
            .cap = actual_cap,
            .buckets = malloc(sizeof(entry*[actual_cap]))
        };

        if(!map->buckets) {
            map->cap = 0;
            return NULL;
        }

        for(size_t i = 0; i < actual_cap; i++) {
            map->buckets[i] = NULL;
        }

        if(pthread_mutex_init(&map->hash_map_mutex, NULL) != 0) {
            hash_map_destroy(map);
            return NULL;
        }
    }

    return map;
}

hash_map* hash_map_put(hash_map* map, char* key, char* value, size_t value_len) {
    if(map) {
        pthread_mutex_lock(&map->hash_map_mutex);
        
        entry* searched_entry = get_entry(map, key);
        
        if(searched_entry) {
            char* old_value = searched_entry->value;
            free(old_value);

            char* new_value = malloc(value_len > 0 ? value_len : 1);
            if(!new_value) {
                pthread_mutex_unlock(&map->hash_map_mutex);
                return NULL;
            }

            memcpy(new_value, value, value_len);
            searched_entry->value = new_value;
            searched_entry->value_len = value_len;

            pthread_mutex_unlock(&map->hash_map_mutex);
            return map;
        }

        double load_factor = (double) map->len / map->cap;
        if(load_factor > 0.75) {
            if(!hash_map_resize(map)) {
                pthread_mutex_unlock(&map->hash_map_mutex);
                return NULL;
            }
        }

        entry* new_entry = malloc(sizeof(entry));
        if(!new_entry) {
            pthread_mutex_unlock(&map->hash_map_mutex);
            return NULL;
        }

        char* copied_key = strdup(key);
        if(!copied_key) {
            free(new_entry);
            pthread_mutex_unlock(&map->hash_map_mutex);
            return NULL;
        }

        char* copied_value = malloc(value_len > 0 ? value_len : 1);
        if(!copied_value) {
            free(copied_key);
            free(new_entry);
            pthread_mutex_unlock(&map->hash_map_mutex);
            return NULL;
        }

        memcpy(copied_value, value, value_len);

        *new_entry = (entry) {
            .key = copied_key,
            .value = copied_value,
            .value_len = value_len,
            .next = NULL,
        };

        if(!hash_map_insert(map, new_entry)) {
            pthread_mutex_unlock(&map->hash_map_mutex);
            return NULL;
        }
        map->len++;

        pthread_mutex_unlock(&map->hash_map_mutex);
        return map;
    }

    return NULL;
}

bool hash_map_remove(hash_map* map, char* key) {
    if(map) {
        pthread_mutex_lock(&map->hash_map_mutex);

        if(!get_entry(map, key)) {
            pthread_mutex_unlock(&map->hash_map_mutex);
            return false;
        }

        size_t bucket_index = hash(key) % map->cap;
        entry* to_remove = map->buckets[bucket_index];

        if(!strcmp(to_remove->key, key)) {
            map->buckets[bucket_index] = to_remove->next;
        } else {
            entry* prev = NULL;

            while(to_remove != NULL) {
                if(!strcmp(to_remove->key, key)) break;
                prev = to_remove;
                to_remove = to_remove->next;
            }

            prev->next = to_remove->next;
        }

        free(to_remove->key);
        free(to_remove->value);
        free(to_remove);
        map->len--;

        pthread_mutex_unlock(&map->hash_map_mutex);
        return true;
    }

    return false;
}

entry_result hash_map_get(hash_map* map, char* key) {
    if(map) {
        pthread_mutex_lock(&map->hash_map_mutex);

        entry* entry_elem = get_entry(map, key);

        if(!entry_elem) {
            pthread_mutex_unlock(&map->hash_map_mutex);

            return (entry_result) {
                .value = NULL,
                .value_len = 0
            };
        }

        char* value = malloc(entry_elem->value_len > 0 ? entry_elem->value_len : 1);
        if(!value) {
            pthread_mutex_unlock(&map->hash_map_mutex);

            return (entry_result) {
                .value = NULL,
                .value_len = 0
            };
        }

        value = memcpy(value, entry_elem->value, entry_elem->value_len);
        size_t value_len = entry_elem->value_len;
    
        pthread_mutex_unlock(&map->hash_map_mutex);

        return (entry_result) {
            .value = value,
            .value_len = value_len
        };
    }

    return (entry_result) {
        .value = NULL,
        .value_len = 0
    };
}