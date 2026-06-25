#include "dynstring.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

void tacc_dynstring_init(tacc_string_p string) {
    string->string = tacc_malloc(1);
    *string->string = 0;
    string->cap = 0;
    string->len = 0;
}

tacc_string_p tacc_dynstring_new(void) {
    struct tacc_string *string;

    string = tacc_malloc(sizeof(struct tacc_string));
    tacc_dynstring_init(string);

    return string;
}

void tacc_dynstring_reset(tacc_string_p string) {
    if (string->string != NULL) {
        tacc_free(string->string);
    }
    tacc_dynstring_init(string);
}

static void tacc_dynstring_grow_to(tacc_string_p string, size_t target_sz) {
    char *end;
    if (string->cap > target_sz) {
        return;
    }
    tacc_assert(target_sz >= string->len,
                "cannot shrink tacc_string below its used length");
    tacc_assert(target_sz != 0, "cannot grow to zero");

    string->string = realloc(string->string, target_sz);
    tacc_assert(string->string != NULL, "failed to realloc string");
    string->cap = target_sz - 1;
    end = string->string + string->cap;
    *end = 0;
}

static void tacc_dynstring_ensure_further_cap(tacc_string_p string,
                                              size_t required_cap) {
    size_t required_full_cap;
    size_t required_cap_p2;

    tacc_assert(0x7FFFFFFF - required_cap > string->len, "overlong string");
    required_full_cap = required_cap + string->len + 1;

    if (string->cap >= required_full_cap) {
        return;
    }
    /*
     * round up to power of 2
     * https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
     */
    required_cap_p2 = required_full_cap - 1;
    required_cap_p2 = required_cap_p2 | (required_cap_p2 >> 1);
    required_cap_p2 = required_cap_p2 | (required_cap_p2 >> 2);
    required_cap_p2 = required_cap_p2 | (required_cap_p2 >> 4);
    required_cap_p2 = required_cap_p2 | (required_cap_p2 >> 8);
    required_cap_p2 = required_cap_p2 | (required_cap_p2 >> 16);
    required_cap_p2 = required_cap_p2 + 1;
    tacc_assert((required_cap_p2 & (required_cap_p2 - 1)) == 0,
                "didn't compute a power of two as cap");
    tacc_assert(required_cap_p2 >= required_full_cap,
                "cap_p2 doesn't cover expected cap");

    tacc_dynstring_grow_to(string, required_cap_p2);
}

void tacc_dynstring_concat(tacc_string_p string, char *continuation) {
    char *old_end;

    tacc_dynstring_ensure_further_cap(string, strlen(continuation));

    old_end = string->string + string->len;
    strcpy(old_end, continuation);
    string->len = string->len + strlen(continuation);
}

void tacc_dynstring_join(tacc_string_p string, tacc_string_p continuation) {
    char *old_end;

    tacc_dynstring_ensure_further_cap(string, continuation->len);

    old_end = string->string + string->len;
    memcpy(old_end, continuation->string, continuation->len + 1);
    string->len = string->len + continuation->len;
}

void tacc_dynstring_push(tacc_string_p string, char to_push) {
    char *old_end;

    tacc_dynstring_ensure_further_cap(string, 1);

    old_end = string->string + string->len;
    string->len = string->len + 1;
    *old_end = to_push;
    old_end = old_end + 1;
    *old_end = 0;
}

char *tacc_dynstring_as_str(tacc_string_p string) { return string->string; }

tacc_string_p tacc_dynstring_clone(tacc_string_p string) {
    tacc_string_p new_string;

    new_string = tacc_malloc(sizeof(struct tacc_string));
    new_string->cap = string->cap;
    new_string->len = string->len;
    new_string->string = tacc_malloc(string->cap + 1);
    memcpy(new_string->string, string->string, string->len + 1);

    return new_string;
}

size_t tacc_dynstring_len(tacc_string_p string) { return string->len; }

char tacc_dynstring_at(tacc_string_p string, size_t index) {
    char *at;

    tacc_assert(index < string->len, "index %u out of bounds", index);
    at = string->string + index;

    return *at;
}

void tacc_dynstring_free(tacc_string_p string) {
    if (string->string != NULL) {
        tacc_free(string->string);
    }
    tacc_free(string);
}

char *tacc_dynstring_take_str(tacc_string_p string) {
    char *str;

    str = string->string;
    tacc_dynstring_init(string);

    return str;
}
