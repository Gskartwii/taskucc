#ifndef TACC_DYNSTRING_H
#define TACC_DYNSTRING_H

#include <stddef.h>

struct tacc_string {
    char *string;
    size_t cap;
    size_t len;
};
typedef struct tacc_string *tacc_string_p;

tacc_string_p tacc_dynstring_new(void);
void tacc_dynstring_init(tacc_string_p string);
void tacc_dynstring_reset(tacc_string_p string);
void tacc_dynstring_concat(tacc_string_p string, char *continuation);
void tacc_dynstring_join(tacc_string_p string, tacc_string_p continuation);
void tacc_dynstring_push(tacc_string_p string, char to_push);
char *tacc_dynstring_as_str(tacc_string_p string);
tacc_string_p tacc_dynstring_clone(tacc_string_p string);
size_t tacc_dynstring_len(tacc_string_p string);
char tacc_dynstring_at(tacc_string_p, size_t index);

#endif
