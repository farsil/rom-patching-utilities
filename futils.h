#ifndef __FUTILS_H
#define __FUTILS_H

#include <stdio.h>

size_t fsize(FILE *stream);

void* fload(size_t size, FILE *stream);

void fsave(void *data, size_t size, FILE *stream);

#endif

