#ifndef TASKU_LIBC_UNISTD_H
#define TASKU_LIBC_UNISTD_H

typedef signed long int ssize_t;

typedef unsigned long int size_t;

int open(const char *path, int flags, ...
         /* mode_t mode */);
ssize_t read(int fd, void *buf, size_t count);

#endif
