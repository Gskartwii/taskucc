#ifndef TASKU_LIBC_STDIO_H
#define TASKU_LIBC_STDIO_H

#define SEEK_SET 0
#define SEEK_END 2

typedef struct {
    int temporary;
} FILE;

extern FILE *const stderr;
extern FILE *const stdout;

typedef char va_list[1];

typedef unsigned long int size_t;

int fclose(FILE *stream);
int fflush(FILE *stream);
int fgetc(FILE *stream);
FILE *fopen(const char *restrict filename, const char *restrict mode);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int fputc(int c, FILE *stream);
int fputs(const char *restrict s, FILE *restrict stream);
size_t
fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
int fseek(FILE *stream, long int offset, int whence);
long int ftell(FILE *stream);
size_t fwrite(const void *restrict ptr,
              size_t size,
              size_t nmemb,
              FILE *restrict stream);
int printf(const char *restrict format, ...);
int sprintf(char *restrict s, const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list arg);
int vsnprintf(char *restrict s,
              size_t n,
              const char *restrict format,
              va_list arg);

FILE *fdopen(int fd, const char *mode);
int remove(const char *path);

#endif
