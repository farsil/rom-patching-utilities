#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <strings.h>

#include "futils.h"

#define IPS_RECORD_RLE 0

const char *usage =
"usage: ipspatch [...options] <patch_file>\n"
"  -h                displays this help message\n"
"  -i source_file    source ROM file, standard input if missing\n"
"  -o target_file    target ROM file, standard output if missing\n"
"  patch_file        patch file in IPS format\n";

uint8_t* decode3(uint8_t *buf, size_t *value) {
    *value = ((buf[0] << 16) & 0xFF0000) + ((buf[1] << 8) & 0xFF00) + buf[2];
    return buf + 3;
}

uint8_t* decode2(uint8_t *buf, size_t *value) {
    *value = ((buf[0] << 8) & 0xFF00) + buf[1];
    return buf + 2;
}

bool pass1(uint8_t *patch_buf, size_t patch_size, size_t *target_size) {
    uint8_t *patch_end = patch_buf + patch_size;
    *target_size = 0;

    if (patch_buf + 5 > patch_end) {
        fputs("Invalid patch record: out of bounds\n", stderr);
        return false;
    }
    if (memcmp(patch_buf, "PATCH", 5) != 0) {
        fputs("Invalid or corrupt patch content\n", stderr);
        return false;
    }
    patch_buf += 5;

    while (true) {
        if (patch_buf + 3 > patch_end) {
            fputs("Invalid patch record: out of bounds\n", stderr);
            return false;
        }
        size_t offset;
        patch_buf = decode3(patch_buf, &offset);

        // checks for the string "EOF"
        if (offset == 0x454F46) {
            if (patch_buf + 3 <= patch_end) {
                // if present, the 3 bytes after EOF hold the target file size
                decode3(patch_buf, target_size);
            }
            return true;
        }

        if (patch_buf + 2 > patch_end) {
            fputs("Invalid patch record: out of bounds\n", stderr);
            return false;
        }
        size_t len;
        patch_buf = decode2(patch_buf, &len);

        if (len == IPS_RECORD_RLE) {
            if (patch_buf + 3 > patch_end) {
                fputs("Invalid patch record: out of bounds\n", stderr);
                return false;
            }
            patch_buf = decode2(patch_buf, &len);
            // ignore RLE value
            patch_buf++;
        } else {
            if (patch_buf + len > patch_end) {
                fputs("Invalid patch record: out of bounds\n", stderr);
                return false;
            }
            patch_buf += len;
        }

        *target_size = *target_size > offset + len
            ? *target_size
            : offset + len;
    }
}

bool pass2(uint8_t *patch_buf, uint8_t *target_buf, size_t target_size) {
    uint8_t *target_end = target_buf + target_size;
    // skip header
    patch_buf += 5;

    while (true) {
        size_t offset;
        patch_buf = decode3(patch_buf, &offset);

        // checks for the string "EOF"
        if (offset == 0x454F46) {
            return true;
        }

        size_t len;
        patch_buf = decode2(patch_buf, &len);

        if (len == IPS_RECORD_RLE) {
            patch_buf = decode2(patch_buf, &len);
            if (target_buf + len > target_end) {
                fputs("Invalid RLE record: out of bounds\n", stderr);
                return false;
            }
            memset(target_buf + offset, *patch_buf, len);
            patch_buf++;
        } else {
            if (target_buf + len > target_end) {
                fputs("Invalid record: out of bounds\n", stderr);
                return false;
            }
            memcpy(target_buf + offset, patch_buf, len);
            patch_buf += len;
        }
    }
}

bool ipspatch(FILE *source_fp, FILE *patch_fp, FILE *target_fp) {
    uint8_t *patch_buf = NULL;
    uint8_t *target_buf = NULL;
    bool success = false;

    size_t patch_size = fsize(patch_fp);
    if (ferror(patch_fp)) {
        perror("Unable to determine patch file size");
        goto end;
    }

    patch_buf = fload(patch_size, patch_fp);
    if (ferror(patch_fp)) {
        perror("Unable to read from patch file");
        goto end;
    }

    size_t target_size;
    if (!pass1(patch_buf, patch_size, &target_size)) {
        goto end;
    }

    target_buf = fload(target_size, source_fp);
    if (ferror(patch_fp)) {
        perror("Unable to read from source file");
        goto end;
    }

    if (!pass2(patch_buf, target_buf, target_size)) {
        goto end;
    }

    fsave(target_buf, target_size, target_fp);
    if (ferror(target_fp)) {
        perror("Unable to write to target file");
        goto end;
    }

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

