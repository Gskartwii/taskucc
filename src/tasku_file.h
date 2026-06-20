#ifndef TASKU_FILE_H
#define TASKU_FILE_H

#include <stddef.h>

struct tacc_file {
    char *name;
    size_t len;
    char *src;
};

typedef struct tacc_file tacc_file_s;
typedef struct tacc_file *tacc_file_p;

tacc_file_p tacc_open(char *name);

#endif
