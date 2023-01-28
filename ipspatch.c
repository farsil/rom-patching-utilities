#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <strings.h>

#define IPS_RECORD_RLE 0

const char *usage =
"usage: ipspatch [...options] <patch_file>\n"
"  -h                displays this help message\n"
"  -i source_file    source ROM file, standard input if missing\n"
"  -o target_file    target ROM file, standard output if missing\n"
"  patch_file        patch file in IPS format\n";

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

void* load(size_t size, FILE *stream) {
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

/* void *load(FILE *stream, size_t *size) { */
/*     *size = 1024 * 1024; */
/*     void *data = malloc(*size); */
/*     size_t offset = 0; */
/*     size_t read, count; */
/*  */
/*     while (true) { */
/*         count = *size - offset < 4096 ? *size - offset : 4096; */
/*         read = fread(data + offset, sizeof *data, count, stream); */
/*         offset += read; */
/*         if (feof(stream) || ferror(stream)) { */
/*             break; */
/*         } */
/*         if (offset == *size) { */
/*             *size <<= 1; */
/*             data = realloc(data, *size); */
/*         } */
/*     }; */
/*  */
/*     if (offset < *size) { */
/*         *size = offset; */
/*         data = realloc(data, *size); */
/*     } */
/*  */
/*     return data; */
/* } */

uint32_t peek_uint24(uint8_t *buf) {
    return ((buf[0] << 16) & 0xFF0000) + ((buf[1] << 8) & 0xFF00) + buf[2];
}

uint16_t peek_uint16(uint8_t *buf) {
    return ((buf[0] << 8) & 0xFF00) + buf[1];
}

size_t pass1(uint8_t *patch_buf, size_t patch_size) {
    uint8_t *patch_end = patch_buf + patch_size;
    size_t target_size = 0;

    if (memcmp(patch_buf, "PATCH", 5) != 0) {
        fputs("Invalid or corrupt patch file\n", stderr);
        return 0;
    }
    patch_buf += 5;

    while (true) {
        if (patch_buf + 3 > patch_end) {
            fputs("Invalid patch record: out of bounds\n", stderr);
            return 0;
        }
        uint32_t offset = peek_uint24(patch_buf);
        patch_buf += 3;

        // checks for the string "EOF"
        if (offset == 0x454F46) {
            if (patch_buf + 3 <= patch_end) {
                // if present, the 3 bytes after EOF hold the target file size
                return peek_uint24(patch_buf);
            }
            return target_size;
        }

        if (patch_buf + 2 > patch_end) {
            fputs("Invalid patch record: out of bounds\n", stderr);
            return 0;
        }
        uint16_t len = peek_uint16(patch_buf);
        patch_buf += 2;

        if (len == IPS_RECORD_RLE) {
            if (patch_buf + 3 > patch_end) {
                fputs("Invalid patch record: out of bounds\n", stderr);
                return 0;
            }   
            len = peek_uint16(patch_buf);
            patch_buf += 3;
        } else {
            if (patch_buf + len > patch_end) {
                fputs("Invalid patch record: out of bounds\n", stderr);
                return 0;
            }   
            patch_buf += len;
        }

        target_size = target_size > offset + len ? target_size : offset + len;
    }

    /* while (patch_buf < patch_end) { */
    /*  */
    /*     if (patch_buf + 3 > patch_end) { */
    /*         fputs("Invalid patch record: out of bounds\n", stderr); */
    /*         return 0; */
    /*     } */
    /*  */
    /*     uint32_t offset = peek_uint24(patch_buf); */
    /*     patch_buf += 3; */
    /*  */
    /*     // EOF */
    /*     if (offset == 0x454F46) { */
    /*         if (patch_buf < patch_end) { */
    /*             return peek_uint24(patch_buf); */
    /*         } */
    /*         return target_size; */
    /*     } */
    /*  */
    /*     uint16_t len = peek_uint16(patch_buf); */
    /*     patch_buf += 2; */
    /*  */
    /*     if (patch_buf + 2 > patch_end) { */
    /*         fputs("Invalid patch record: out of bounds\n", stderr); */
    /*         return 0; */
    /*     } */
    /*     size_t end_offset; */
    /*     if (len == IPS_RECORD_RLE) { */
    /*         if (patch_buf + 3 > patch_end) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             return 0; */
    /*         }    */
    /*         uint16_t rle_len = peek_uint16(patch_buf); */
    /*         end_offset = offset + rle_len; */
    /*         patch_buf += 3; */
    /*     } else { */
    /*         if (patch_buf + len > patch_end) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             return 0; */
    /*         }    */
    /*         end_offset = offset + len; */
    /*         patch_buf += len; */
    /*     } */
    /*  */
    /*     target_size = target_size > end_offset ? target_size : end_offset; */
    /* } */

    /* return 0; */
    /* while (memcmp(patch_buf + patch_off, "EOF", 3) != 0) { */
    /*     if (patch_off + 5 >= patch_size) { */
    /*         fputs("Invalid patch record: out of bounds\n", stderr); */
    /*         return 0; */
    /*     } */
    /*  */
    /*     // big endian */
    /*     uint32_t offset = peek_uint24(patch_buf + patch_off); */
    /*     patch_off += 3; */
    /*     uint16_t len = peek_uint16(patch_buf + patch_off); */
    /*     patch_off += 2; */
    /*     size_t end_offset; */
    /*  */
    /*         fprintf(stderr, "%zu %zu %zu\n", patch_off, len, patch_size); */
    /*     if (len == IPS_RECORD_RLE) { */
    /*         if (patch_off + 3 >= patch_size) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             return 0; */
    /*         } */
    /*         uint16_t rle_len = peek_uint16(patch_buf + patch_off); */
    /*         end_offset = offset + rle_len; */
    /*         patch_off += 3; */
    /*     } else { */
    /*         if (patch_off + len >= patch_size) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             return 0; */
    /*         } */
    /*         end_offset = offset + len; */
    /*         patch_off += len; */
    /*     } */
    /*      */
    /*     target_size = target_size > end_offset ? target_size : end_offset; */
    /* } */

    /* patch_off += 3; */
    /*  */
    /* // if present, the three bytes after EOF hold the target file size */
    /* if (patch_size >= patch_off + 3) { */
    /*     target_size = peek_uint24(patch_buf + patch_off); */
    /* } */
    /*  */
    /* return target_size; */
}

bool pass2(uint8_t *patch_buf, size_t patch_size, uint8_t *target_buf, size_t target_size) {
    return true;
}

void save(void *data, size_t size, FILE *stream) {
    size_t offset = 0;
    size_t written, count;

    do {
        count = size - offset < 4096 ? size - offset : 4096;
        written = fwrite(data + offset, sizeof *data, count, stream);
        offset += written;
    } while (written == count && offset < size);
}

/* size_t decode(uint8_t *data, size_t offset, size_t size, size_t *value) { */
/*     size_t shift = 0; */
/*     size_t read = 0; */
/*     *value = 0; */
/*  */
/*     while (offset + read < size) { */
/*         uint8_t next = data[offset + read++]; */
/*         *value += (next ^ 0x80) << shift; */
/*         if (next & 0x80) return read; */
/*         shift += 7; */
/*     }; */
/*  */
/*     return read; */
/* } */

bool ipspatch(FILE *source_fp, FILE *patch_fp, FILE *target_fp) {
    /* uint8_t *source_buf = NULL; */
    uint8_t *patch_buf = NULL;
    uint8_t *target_buf = NULL;
    bool success = false;

    size_t patch_size = fsize(patch_fp);
    if (ferror(patch_fp)) {
        perror("Unable to determine patch file size");
        goto end;
    }

    patch_buf = load(patch_size, patch_fp);
    if (ferror(patch_fp)) {
        perror("Unable to read from patch file");
        goto end;
    }

    size_t target_size = pass1(patch_buf, patch_size);
    fprintf(stderr, "RETVAL: %lu\n", target_size);
    if (target_size == 0) {
        goto end;
    }



    /* target_buf = load(target_size, source_fp); */
    /* success = pass2(patch_buf, patch_size, target_buf, target_size); */


    /* size_t target_off = 0; */
    /* while (!feof(source_fp) && target_off < target_size) { */
    /*     size_t count = target_size - offset < 4096 ? target_size - offset : 4096; */
    /*     size_t read = fread(target_buf + target_off,  */
    /*                         sizeof *target_buf, count, target_fp); */
    /*     target_off += read; */
    /* } */

    /* if (memcmp(patch_buf, "PATCH", 5) != 0) { */
    /*     fputs("Invalid or corrupt patch file\n", stderr); */
    /*     goto end; */
    /* } */
    /*  */
    /* size_t patch_off = 5; */
    /* while (memcmp(patch_buf + patch_off, "EOF", 3) != 0) { */
    /*     if (patch_off + 5 >= patch_size) { */
    /*         fputs("Invalid patch record: out of bounds\n", stderr); */
    /*         goto end; */
    /*     } */
    /*  */
    /*     // big endian */
    /*     uint32_t offset = (patch_buf[patch_off] << 16) & 0xFF0000 + */
    /*                       (patch_buf[patch_off + 1] << 8) & 0xFF00 + */
    /*                       patch_buf[patch_off + 2]; */
    /*     uint16_t len = (patch_buf[patch_off + 3] << 8) & 0xFF00 + */
    /*                    patch_buf[patch_off + 4]; */
    /*     patch_off += 5; */
    /*  */
    /*     if (len == IPS_RECORD_RLE) { */
    /*         if (patch_off + 3 >= patch_size) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             goto end; */
    /*         } */
    /*  */
    /*         uint16_t rle_len = (patch_buf[patch_off + 5] << 8) & 0xFF00 + */
    /*                            patch_buf[patch_off + 6]; */
    /*         uint8_t byte = patch_buf[patch_off + 7]; */
    /*  */
    /*         memset(target_buf + target_off, byte, len); */
    /*         patch_off += 3; */
    /*     } else { */
    /*         if (patch_off + len >= patch_size) { */
    /*             fputs("Invalid patch record: out of bounds\n", stderr); */
    /*             goto end; */
    /*         } */
    /*         memcpy(target_buf + target_off, patch_buf + patch_off, len); */
    /*         patch_off += len; */
    /*     } */
    /* } */

    success = true;

end:
    free(patch_buf);
    free(target_buf);
    return success;
}

int main(int argc, char *argv[]) {
    FILE *source = stdin;
    FILE *patch = NULL;
    FILE *target = stdout;
    int exitcode = EXIT_FAILURE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            fputs(usage, stdout);
            exitcode = EXIT_SUCCESS;
            goto end;
        }
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            source = fopen(argv[++i], "rb");
            if (source == NULL) {
                fprintf(stderr, "Unable to open source file %s: %s\n", 
                        argv[i], strerror(errno));
                goto end;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            target = fopen(argv[++i], "wb");
            if (target == NULL) {
                fprintf(stderr, "Unable to open target file %s: %s\n", 
                        argv[i], strerror(errno));
                goto end;
            }
        }
        else if (i == argc - 1) {
            patch = fopen(argv[i], "rb");
            if (patch == NULL) {
                fprintf(stderr, "Unable to open patch file %s: %s\n", 
                        argv[i], strerror(errno));
                goto end;
            }
        }
        else {
            fprintf(stderr, "ipspatch: unrecognized option '%s'\n", argv[1]);
            fputs(usage, stderr);
            goto end;
        }
    }

    if (patch == NULL) {
        fputs(usage, stderr);
        goto end;
    }

    if (ipspatch(source, patch, target)) {
        exitcode = EXIT_SUCCESS;
    }

end:
    if (source != stdin) fclose(source);
    fclose(patch);
    if (target != stdout) fclose(target);
    return exitcode;
}
