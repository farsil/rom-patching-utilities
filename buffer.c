#include <stdlib.h>

void *balloc(size_t size) {
    void *ptr = malloc(sizeof(size_t) + size);
    *(size_t*)ptr = size;
    return ptr + sizeof(size_t);
}

void bfree(void *ptr) {
    if (ptr != NULL) {
        free(ptr - sizeof(size_t));
    }
}

size_t blen(void *buf) {
    return *(size_t*)(buf - sizeof(size_t));
}
