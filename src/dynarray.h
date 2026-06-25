#ifndef TACC_DYNARRAY_H
#define TACC_DYNARRAY_H

#include <stddef.h>

struct tacc_dynarray {
    void *buffer;
    size_t cap;
    size_t len;
    size_t element_size;
};

struct tacc_dynarray *tacc_dynarray_new(size_t element_size);
void tacc_dynarray_init(struct tacc_dynarray *array, size_t element_size);
void *tacc_dynarray_get(struct tacc_dynarray *array, size_t index);
void tacc_dynarray_push(struct tacc_dynarray *array, void *elem);
void tacc_dynarray_pop(struct tacc_dynarray *array);
size_t tacc_dynarray_len(struct tacc_dynarray *array);
struct tacc_dynarray *tacc_dynarray_clone(struct tacc_dynarray *array);
void tacc_dynarray_free(struct tacc_dynarray *array);
void tacc_dynarray_reset(struct tacc_dynarray *array);

#endif
