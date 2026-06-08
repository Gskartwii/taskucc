#include <stdarg.h>
#include <stdio.h>

void die(char *err, ...) {
    va_list args;

    va_start(args, err);

    vfprintf(stderr, err, args);
    __kill_io();
    exit(1);
}

int main(int argc, char **argv) {
    char *filename;

    __init_io();

    if (argc < 2) {
        die("need filename\n");
    }
    filename = argv[1];
    die("testing filename: %s\n", filename);

    return 0;
}
