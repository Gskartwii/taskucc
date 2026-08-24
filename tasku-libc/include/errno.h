#ifndef TASKU_LIBC_ERRNO_H
#define TASKU_LIBC_ERRNO_H

int __tasku_libc_errno;

#define errno __tasku_libc_errno

#endif
