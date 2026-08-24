#ifndef TASKU_LIBC_SETJMP_H
#define TASKU_LIBC_SETJMP_H

/* not final */
typedef long jmp_buf[4];

void longjmp(jmp_buf env, int val);
int setjmp(jmp_buf env);

#endif
