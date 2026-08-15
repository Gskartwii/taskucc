#ifndef TACC_UTIL_H
#define TACC_UTIL_H

#include "gcc_compat.h"
#include <stddef.h>
#include <stdint.h>

/* M2 unconditionally sign-extends 0xffffffff on x86_64, but might store x in a
 * zero-extended register. So a naive comparison doesn't work. */
#define IS_U32_MAX(x)                                                \
    ((((x) & 0x7FFFFFFF) == 0x7FFFFFFF) && ((((x) >> 31) & 1) == 1))

#ifdef __M2__
#define TACC_UNUSED(x) 0
#else
#define TACC_UNUSED(x) (void) x
#endif

typedef int tacc_bool;

void tacc_die(char *err, ...);
void *tacc_malloc(size_t sz);
void tacc_free(void *allocation);
uint8_t tacc_hex_to_dec(char hex);
size_t tacc_sizeadj(size_t count, size_t size);

#endif
