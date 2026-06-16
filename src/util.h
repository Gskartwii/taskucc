#ifndef TACC_UTIL_H
#define TACC_UTIL_H

#include <stddef.h>

void tacc_die(char *err, ...);
void tacc_assert(int cond, char *err, ...);
void *tacc_malloc(size_t sz);
int tacc_hex_to_dec(char hex);
int tacc_sizeadj(int count, int size);

#endif
