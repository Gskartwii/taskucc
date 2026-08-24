#ifndef TASKU_LIBC_STDARG_H
#define TASKU_LIBC_STDARG_H

typedef char va_list[1];

#define va_arg(x, y) x
#define va_start(x, p)
#define va_copy(x, y)
#define va_end(x)

#endif
