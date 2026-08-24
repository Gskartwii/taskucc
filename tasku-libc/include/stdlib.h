#ifndef TASKU_LIBC_STDLIB_H
#define TASKU_LIBC_STDLIB_H

typedef unsigned long int size_t;

void qsort(void *base,
           size_t nmemb,
           size_t size,
           int (*compar)(const void *, const void *));
double strtod(const char *restrict nptr, char **restrict endptr);

#endif
