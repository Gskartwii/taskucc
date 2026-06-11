#ifndef TACC_UTIL_H
#define TACC_UTIL_H

void tacc_die(char *err, ...);
void tacc_assert(int cond, char *err, ...);
void *tacc_malloc(size_t sz);

#endif
