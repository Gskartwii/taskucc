#ifndef TASKU_FILE_H
#define TASKU_FILE_H

#include <stddef.h>

struct tacc_file {
    char *tacc_file_name;
    size_t tacc_file_len;
    char *tacc_file_src;
};

typedef struct tacc_file tacc_file_s;
typedef struct tacc_file *tacc_file_p;

tacc_file_p tacc_open(char *name);

#endif
