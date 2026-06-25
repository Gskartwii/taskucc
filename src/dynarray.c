#include "dynarray.h"
#include "gcc_compat.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

void tacc_dynarray_init(struct tacc_dynarray *array, size_t elem_size) {
    array->element_size = elem_size;
    array->buffer = tacc_malloc(elem_size);
    array->cap = 1;
    array->len = 0;
}

struct tacc_dynarray *tacc_dynarray_new(size_t elem_size) {
    struct tacc_dynarray *array;

    array = tacc_malloc(sizeof(struct tacc_dynarray));
    tacc_dynarray_init(array, elem_size);

    return array;
}

static void tacc_dynarray_grow_to(struct tacc_dynarray *array,
                                  size_t target_sz) {
    if (array->cap > target_sz) {
        return;
    }
    tacc_assert(target_sz >= array->len,
                "cannot shrink tacc_dynarray below its used length");
    tacc_assert(target_sz != 0, "cannot grow array to zero");

    array->buffer =
        realloc(array->buffer, target_sz * array->element_size + 15);
    array->buffer =
        (void *) ((((uintptr_t) (array->buffer)) + 15) & ~(uintptr_t) 0xF);
    tacc_assert(array->buffer != NULL, "failed to realloc array");
    array->cap = target_sz;
}

static void tacc_dynarray_ensure_further_cap(struct tacc_dynarray *array,
                                             size_t required_cap) {
    size_t required_full_cap;
    size_t required_cap_p2;

    tacc_assert(0x7FFFFFFF / array->element_size - required_cap > array->len,
                "overlong array");

    required_full_cap = required_cap + array->len;
    if (array->cap >= required_full_cap) {
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

    tacc_dynarray_grow_to(array, required_cap_p2);
}
void *tacc_dynarray_get(struct tacc_dynarray *array, size_t index) {
    char *buf;

    tacc_assert(array->len > index,
                "index %" PRIsz " out of bounds for %" PRIsz,
                index,
                array->len);
    buf = array->buffer;

    return buf + index * array->element_size;
}
void tacc_dynarray_push(struct tacc_dynarray *array, void *elem) {
    char *old_end;
    char *buf;

    tacc_dynarray_ensure_further_cap(array, 1);
    buf = array->buffer;
    old_end = buf + array->len * array->element_size;
    memcpy(old_end, elem, array->element_size);
    array->len = array->len + 1;
}
void tacc_dynarray_pop(struct tacc_dynarray *array) {
    tacc_assert(array->len > 0, "cannot pop from empty array");
    array->len = array->len - 1;
}
size_t tacc_dynarray_len(struct tacc_dynarray *array) { return array->len; }
struct tacc_dynarray *tacc_dynarray_clone(struct tacc_dynarray *array) {
    struct tacc_dynarray *new;

    new = tacc_malloc(sizeof(struct tacc_dynarray));
    new->cap = array->cap;
    new->element_size = array->element_size;
    new->len = array->len;
    new->buffer = tacc_malloc(array->element_size * array->cap);
    memcpy(new->buffer, array->buffer, array->element_size * array->cap);

    return new;
}

void tacc_dynarray_free(struct tacc_dynarray *array) {
    tacc_free(array->buffer);
    tacc_free(array);
}
