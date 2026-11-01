#ifndef TASKU_LIBC_STDARG_H
#define TASKU_LIBC_STDARG_H

typedef __builtin_va_list va_list;

#define va_arg(x, y) __builtin_va_arg(x, y)
#define va_start(x, p) __builtin_va_start(x, p)
#define va_copy(x, y) __builtin_va_copy(x, y)
#define va_end(x) __builtin_va_end(x)

#endif
