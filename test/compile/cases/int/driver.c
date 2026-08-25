#include <stdio.h>

extern int simple(int);

int main(void) {
    int ret_val = simple(0);

    printf("ret_val = %d\n", ret_val);
    return !(ret_val == 42);
}
