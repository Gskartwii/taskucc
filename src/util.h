#ifndef TACC_UTIL_H
#define TACC_UTIL_H

#include "gcc_compat.h"
#include <stddef.h>
#include <stdint.h>

typedef int tacc_bool;

void tacc_die(char *err, ...);
void *tacc_malloc(size_t sz);
void tacc_free(void *allocation);
uint8_t tacc_hex_to_dec(char hex);
size_t tacc_sizeadj(size_t count, size_t size);

#endif
