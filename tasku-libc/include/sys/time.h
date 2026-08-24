#ifndef TASKU_LIBC_SYS_TIME_H
#define TASKU_LIBC_SYS_TIME_H

typedef struct {
    int temporary;
} time_t;

int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);

#endif
