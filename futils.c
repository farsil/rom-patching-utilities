#include <stdlib.h>

#include "futils.h"

size_t fsize(FILE *stream) {
    long curpos = ftell(stream);
    if (curpos == -1L) {
        return 0;
    }

    if (fseek(stream, 0L, SEEK_END)) {
        return 0;
    }

    long endpos = ftell(stream);
    if (endpos == -1L) {
        return 0;
    }

    if (fseek(stream, curpos, SEEK_SET)) {
        return 0;
    }

    // not portable on the few systems where LONG_MAX > SIZE_MAX
    return endpos;
}

void* fload(size_t size, FILE *stream) {
    void *data = malloc(size);
    size_t offset = 0;
    size_t read, count;

    do {
        count = size - offset < 4096 ? size - offset : 4096;
        read = fread(data + offset, sizeof *data, count, stream);
        offset += read;
    } while (read == count && offset < size);

    return data;
}

void fsave(void *data, size_t size, FILE *stream) {
    size_t offset = 0;
    size_t written, count;

    do {
        count = size - offset < 4096 ? size - offset : 4096;
        written = fwrite(data + offset, sizeof *data, count, stream);
        offset += written;
    } while (written == count && offset < size);
}

