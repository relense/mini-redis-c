#include "byte-buffer.h"
#include <string.h>
#include <assert.h>

int main(void) {
    byte_buffer bbuffer;
    byte_buffer_init(&bbuffer, 0);

    char* data = "Hello World";
    byte_buffer_append(&bbuffer, data, 11);

    assert(bbuffer.len == 11);
    assert(memcmp(bbuffer.data, "Hello World", 11) == 0);
}