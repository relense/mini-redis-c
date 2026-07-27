#include "byte-buffer.h"
#include <string.h>
#include <stdlib.h>

static byte_buffer* byte_buffer_resize(byte_buffer* bbuffer, size_t cap) {
    if(bbuffer && cap > 0 && cap >= bbuffer->len) {
        char* data = realloc(bbuffer->data, sizeof(char[cap]));

        if(!data) return NULL;

        bbuffer->cap = cap;
        bbuffer->data = data;

        return bbuffer;
    }

    return NULL;
}

static byte_buffer* ensure_capacity(byte_buffer* bbuffer, size_t byte_len) {
    if(bbuffer) {
        if(bbuffer->len + byte_len < bbuffer->cap) {
            return bbuffer;
        }
        
        size_t new_cap = bbuffer->cap == 0 ? 16 : bbuffer->cap;

        while (bbuffer->len + byte_len >= new_cap) {
            new_cap *= 2;
        }

        if(!byte_buffer_resize(bbuffer, new_cap)) return NULL;

        return bbuffer;
    }

    return NULL;
}

byte_buffer* byte_buffer_init(byte_buffer* bbuffer, size_t cap) {
    if(bbuffer) {
        if(cap) {
            *bbuffer = (byte_buffer) {
                .cap = cap,
                .data = malloc(sizeof(char[cap])),
            };

            if(!bbuffer->data) {
                bbuffer->cap = 0;
                return NULL;
            }
        } else {
            *bbuffer = (byte_buffer) {};
        }
    }

    return bbuffer;
}

void byte_buffer_destroy(byte_buffer* bbuffer) {
    if(bbuffer) {
        free(bbuffer->data);
        *bbuffer = (byte_buffer) {};
    }
};

byte_buffer* byte_buffer_append(byte_buffer* bbuffer, const char* data, size_t byte_len) {
    if(bbuffer) {
        if(!ensure_capacity(bbuffer, byte_len)) return NULL;
        
        memcpy(bbuffer->data + bbuffer->len, data, byte_len);
        bbuffer->len += byte_len;
        
        return bbuffer;
    }

    return NULL;
}

void byte_buffer_reset(byte_buffer* bbuffer) {
    if(bbuffer) {
        bbuffer->len = 0;
    }
}