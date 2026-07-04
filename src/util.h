#ifndef TACC_UTIL_H
#define TACC_UTIL_H

#include <stddef.h>
#include <stdint.h>

typedef int tacc_bool;

void tacc_die(char *err, ...);
#define tacc_assert(cond, ...) \
    if (!(cond)) {             \
        tacc_die(__VA_ARGS__); \
    }
void *tacc_malloc(size_t sz);
void tacc_free(void *allocation);
uint8_t tacc_hex_to_dec(char hex);
size_t tacc_sizeadj(size_t count, size_t size);

#endif
