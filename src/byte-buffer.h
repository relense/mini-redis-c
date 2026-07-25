/*
* byte-buffer.h
*
* A dynamically growing buffer of raw bytes, used to accumulate data
* received from recv() until a complete RESP message is available.
*/
#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <stdlib.h>

typedef struct byte_buffer byte_buffer;

struct byte_buffer {
    size_t len;
    size_t cap;
    char* data;
};

/*
* Initializes bbuffer with the given capacity. Returns bbuffer on success,
* or NULL if allocation failed (bbuffer itself remains a valid, zeroed struct
* in that case, but with no capacity).
*/
byte_buffer* byte_buffer_init(byte_buffer* bbuffer, size_t cap);
void byte_buffer_destroy(byte_buffer* bbuffer);

/*
* Appends byte_len bytes from data to bbuffer, growing it if needed.
* Returns bbuffer on success, or NULL on allocation failure.
*
* IMPORTANT: if this returns NULL, bbuffer itself is still valid and
* unchanged (the append simply didn't happen). Do NOT overwrite your
* own pointer/struct with the return value directly:
*     bbuffer = byte_buffer_append(&bbuffer, data, len);   // WRONG if it can return NULL
*
* Instead, check the result separately:
*     if (!byte_buffer_append(&bbuffer, data, len)) { // handle error }
*/
byte_buffer* byte_buffer_append(byte_buffer* bbuffer, const char* data, size_t byte_len);

#endif