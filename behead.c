#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <strings.h>

const char *usage =
"usage: behead [...options]\n"
"  -h                displays this help message\n"
"  -i source_file    source ROM file, standard input if missing\n"
"  -o target_file    target ROM file, standard output if missing\n";

bool behead(FILE *source_fp, FILE *target_fp) {
    uint8_t buf[4096];

    // discard first 512 bytes, not using fseek because source_fp may be stdin
    fread(buf, sizeof *buf, 512, source_fp);
    if (ferror(source_fp)) {
        perror("Unable to read from source file");
        return false;
    }
    
    size_t read;
    do {
        read = fread(buf, sizeof *buf, 4096, source_fp);
        if (ferror(source_fp)) {
            perror("Unable to read from source file");
            return false;
        }
        fwrite(buf, sizeof *buf, read, target_fp);
        if (ferror(target_fp)) {
            perror("Unable to write to target file");
            return false;
        }
    } while (read == 4096);
    
    return true;
}

int main(int argc, char *argv[]) {
    FILE *source = stdin;
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
        else {
            fprintf(stderr, "behead: unrecognized option '%s'\n", argv[1]);
            fputs(usage, stderr);
            goto end;
        }
    }

    if (behead(source, target)) {
        exitcode = EXIT_SUCCESS;
    }

end:
    if (source != stdin) fclose(source);
    if (target != stdout) fclose(target);
    return exitcode;
}
