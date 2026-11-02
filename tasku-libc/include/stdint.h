#ifndef TASKU_LIBC_STDINT_H
#define TASKU_LIBC_STDINT_H

#include <stddef.h>

typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long long int int64_t;
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#ifdef __i386__
typedef unsigned long long int uint64_t;
#define SIZE_MAX 0xFFFFFFFF
#else
#define SIZE_MAX 0xFFFFFFFFFFFFFFFF
typedef unsigned long int uint64_t;
#endif

#endif
