#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "gcc_compat.h"
#include "util.h"

struct tacc_file {
    const char *name;
    size_t len;
    const char *_src;
};

typedef struct tacc_file tacc_file_s;
typedef struct tacc_file *tacc_file_p;

tacc_file_p tacc_open(const char *name) {
    FILE *f;
    char *src;
    long file_sz;
    size_t read_sz;
    tacc_file_p out_file;

    f = fopen(name, "r");
    tacc_assert(f != NULL, "failed to open file %s: %d", name, errno);

    /*
     * ignore return value; should return 0 on success per standard, but m2libc
     * returns cursor position instead
     */
    fseek(f, 0, SEEK_END);

    file_sz = ftell(f);
    tacc_assert(file_sz != -1, "failed to get file %s size: %d", name, errno);

    fseek(f, 0, SEEK_SET);

    out_file = tacc_malloc(sizeof(tacc_file_s));
    src = tacc_malloc((file_sz + 1) * sizeof(char));

    read_sz = fread(src, sizeof(char), file_sz, f);
    tacc_assert(read_sz == file_sz,
                "failed to read file %s: got %zu bytes with errno %d",
                name, read_sz, errno);
    tacc_assert(fclose(f) == 0, "failed to close file %s: %d", name, errno);

    src[file_sz] = 0;

    out_file->_src = src;
    out_file->name = name;
    out_file->len = file_sz;

    return out_file;
}

int main(int argc, char **argv) {
    char *filename;
    struct tacc_file *input_file;

    init_io();

    tacc_assert(argc >= 2, "need filename");
    filename = argv[1];

    input_file = tacc_open(filename);

    printf("%s\n", input_file->_src);

    return 0;
}
