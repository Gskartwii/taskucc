#include <stdio.h>

extern int simple(void);

int main(void) {
    int ret_val = simple();

    printf("ret_val = %d\n", ret_val);
    return !(ret_val == 42);
}
