#ifndef TASKU_LIBC_STDIO_H
#define TASKU_LIBC_STDIO_H

typedef struct {
    int temporary;
} FILE;

typedef char va_list[1];

typedef unsigned long int size_t;

int fflush(FILE *stream);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int fputs(const char *restrict s, FILE *restrict stream);
int printf(const char *restrict format, ...);
int sprintf(char *restrict s, const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list arg);
int vsnprintf(char *restrict s,
              size_t n,
              const char *restrict format,
              va_list arg);

#endif
