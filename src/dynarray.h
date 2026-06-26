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
void tacc_dynarray_pop(struct tacc_dynarray *array, void *out);
size_t tacc_dynarray_len(struct tacc_dynarray *array);
struct tacc_dynarray *tacc_dynarray_clone(struct tacc_dynarray *array);
void tacc_dynarray_free(struct tacc_dynarray *array);
void tacc_dynarray_reset(struct tacc_dynarray *array);

#define DECL_DYNARRAY_OVER(arr_type,                                          \
                           elem_wrapper_type,                                 \
                           elem_type,                                         \
                           new_func,                                          \
                           init_func,                                         \
                           get_func,                                          \
                           push_func,                                         \
                           pop_func,                                          \
                           len_func,                                          \
                           free_func)                                         \
    struct elem_wrapper_type {                                                \
        elem_type content;                                                    \
    };                                                                        \
    struct arr_type {                                                         \
        struct tacc_dynarray *list;                                           \
    };                                                                        \
    struct arr_type *new_func(void);                                          \
    void init_func(struct arr_type *array);                                   \
    struct elem_wrapper_type *get_func(struct arr_type *array, size_t index); \
    void push_func(struct arr_type *array, elem_type content);                \
    elem_type pop_func(struct arr_type *array);                               \
    size_t len_func(struct arr_type *array);                                  \
    void free_func(struct arr_type *array);

#define MK_DYNARRAY_OVER(arr_type,                                             \
                         elem_wrapper_type,                                    \
                         elem_type,                                            \
                         new_func,                                             \
                         init_func,                                            \
                         get_func,                                             \
                         push_func,                                            \
                         pop_func,                                             \
                         len_func,                                             \
                         deinit_func,                                          \
                         free_func)                                            \
    struct arr_type *new_func(void) {                                          \
        struct arr_type *list;                                                 \
                                                                               \
        list = tacc_malloc(sizeof(struct arr_type));                           \
        list->list = tacc_dynarray_new(sizeof(struct elem_wrapper_type));      \
                                                                               \
        return list;                                                           \
    }                                                                          \
    void init_func(struct arr_type *array) {                                   \
        array->list = tacc_dynarray_new(sizeof(struct elem_wrapper_type));     \
    }                                                                          \
    struct elem_wrapper_type *get_func(struct arr_type *array, size_t index) { \
        return tacc_dynarray_get(array->list, index);                          \
    }                                                                          \
    void push_func(struct arr_type *array, elem_type content) {                \
        struct elem_wrapper_type wrapper;                                      \
        wrapper.content = content;                                             \
        tacc_dynarray_push(array->list, &wrapper);                             \
    }                                                                          \
    elem_type pop_func(struct arr_type *array) {                               \
        struct elem_wrapper_type wrapper;                                      \
        tacc_dynarray_pop(array->list, &wrapper);                              \
        return wrapper.content;                                                \
    }                                                                          \
    size_t len_func(struct arr_type *array) {                                  \
        return tacc_dynarray_len(array->list);                                 \
    }                                                                          \
    void free_func(struct arr_type *array) {                                   \
        if (deinit_func != NULL) {                                             \
            while (len_func(array) > 0) {                                      \
                deinit_func(pop_func(array));                                  \
            }                                                                  \
        }                                                                      \
        tacc_dynarray_free(array->list);                                       \
    }

#endif
