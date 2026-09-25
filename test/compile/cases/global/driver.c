#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

extern int get_global(void);
extern int set_global(int);
extern int indirect_access(void);
extern int indirect_set(int);

int main(void) {
    _Bool ok = true;

    if (get_global() != 4) {
        printf("initial != 4");
        ok = false;
    }
    if (get_global() != 4) {
        printf("initial (again) != 4");
        ok = false;
    }
    if (indirect_access() != 4) {
        printf("initial (indirect) != 4");
        ok = false;
    }
    set_global(5);
    if (get_global() != 5) {
        printf("updated != 5");
        ok = false;
    }
    if (get_global() != 5) {
        printf("updated (again) != 5");
        ok = false;
    }
    if (indirect_access() != 5) {
        printf("updated (indirect) != 5");
        ok = false;
    }

    indirect_set(6);
    if (indirect_access() != 6) {
        printf("indirectly updated (indirect) != 6");
        ok = false;
    }

    return !ok;
}
