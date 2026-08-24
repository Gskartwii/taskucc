#ifndef TASKU_LIBC_STDLIB_H
#define TASKU_LIBC_STDLIB_H

typedef unsigned long int size_t;

int atoi(const char *nptr);
void exit(int status);
void free(void *ptr);
void qsort(void *base,
           size_t nmemb,
           size_t size,
           int (*compar)(const void *, const void *));
void *realloc(void *ptr, size_t size);
double strtod(const char *restrict nptr, char **restrict endptr);
long int strtol(const char *restrict nptr, char **restrict endptr, int base);
unsigned long int strtoul(const char *restrict nptr,
                          char **restrict endptr,
                          int base);
unsigned long long int strtoull(const char *restrict nptr,
                                char **restrict endptr,
                                int base);

char *realpath(const char *restrict path, char *restrict resolved_path);

#endif
