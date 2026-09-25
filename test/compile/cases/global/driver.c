#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

extern int get_global(void);
extern int set_global(int);
extern int indirect_access(void);

int main(void) {
    _Bool ok = true;

    if (get_global() != 4) {
        printf("initial != 4");
    }
    if (get_global() != 4) {
        printf("initial (again) != 4");
    }
    if (indirect_access() != 4) {
        printf("initial (indirect) != 4");
    }
    set_global(5);
    if (get_global() != 5) {
        printf("updated != 5");
    }
    if (get_global() != 5) {
        printf("updated (again) != 5");
    }
    if (indirect_access() != 5) {
        printf("updated (indirect) != 5");
    }

    return !ok;
}
