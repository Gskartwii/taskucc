#ifndef TACC_DYNHASH_H
#define TACC_DYNHASH_H

#include <inttypes.h>
#include <stddef.h>

struct tacc_dynhash {
    void *buffer;
    size_t cap;
    size_t fill;
    size_t element_size;
};

uint32_t tacc_str_hash(char *name);
struct tacc_dynhash *tacc_dynhash_new(size_t cap, size_t element_size);
void tacc_dynhash_init(struct tacc_dynhash *hash,
                       size_t cap,
                       size_t element_size);
void *tacc_dynhash_probe(struct tacc_dynhash *hash, size_t index);
void tacc_dynhash_insert_new(struct tacc_dynhash *hash,
                             size_t index,
                             void *elem);
size_t tacc_dynhash_fill_count(struct tacc_dynhash *hash);
void tacc_dynhash_free(struct tacc_dynhash *hash);

#define DECL_DYNHASH_OVER(hash_type,                                        \
                          elem_wrapper_type,                                \
                          elem_type,                                        \
                          new_func,                                         \
                          init_func,                                        \
                          get_func,                                         \
                          insert_func,                                      \
                          fill_count_func,                                  \
                          free_func)                                        \
    struct elem_wrapper_type {                                              \
        elem_type content;                                                  \
    };                                                                      \
    struct hash_type {                                                      \
        struct tacc_dynhash *map;                                           \
    };                                                                      \
    struct hash_type *new_func(size_t cap);                                 \
    void init_func(struct hash_type *array, size_t cap);                    \
    struct elem_wrapper_type *get_func(struct hash_type *array, char *key); \
    void insert_func(struct hash_type *array, elem_type content);           \
    size_t fill_count_func(struct hash_type *array);                        \
    void free_func(struct hash_type *array);

#define MK_DYNHASH_OVER(hash_type,                                          \
                        hash_key,                                           \
                        elem_wrapper_type,                                  \
                        elem_type,                                          \
                        new_func,                                           \
                        init_func,                                          \
                        get_func,                                           \
                        insert_func,                                        \
                        fill_count_func,                                    \
                        deinit_func,                                        \
                        free_func)                                          \
    struct hash_type *new_func(size_t cap) {                                \
        struct hash_type *map;                                              \
                                                                            \
        map = tacc_malloc(sizeof(struct hash_type));                        \
        init_func(map, cap);                                                \
                                                                            \
        return map;                                                         \
    }                                                                       \
    void init_func(struct hash_type *map, size_t cap) {                     \
        size_t i;                                                           \
        struct elem_wrapper_type wrapper;                                   \
                                                                            \
        wrapper.content = NULL;                                             \
        map->map = tacc_dynhash_new(cap, sizeof(struct elem_wrapper_type)); \
        for (i = 0; i < cap; i = i + 1) {                                   \
            tacc_dynhash_insert_new(map->map, i, &wrapper);                 \
        }                                                                   \
        map->map->fill = 0;                                                 \
    }                                                                       \
    struct elem_wrapper_type *get_func(struct hash_type *map, char *key) {  \
        size_t i;                                                           \
        struct elem_wrapper_type *probe;                                    \
        elem_type content;                                                  \
                                                                            \
        i = tacc_str_hash(key);                                             \
        while (1) {                                                         \
            probe = tacc_dynhash_probe(map->map, i);                        \
            if (probe->content == NULL) {                                   \
                break;                                                      \
            }                                                               \
            content = probe->content;                                       \
            if (!strcmp(content->hash_key, key)) {                          \
                return probe;                                               \
            }                                                               \
            i = i + 1;                                                      \
        }                                                                   \
        return NULL;                                                        \
    }                                                                       \
    void insert_func(struct hash_type *map, elem_type content) {            \
        struct elem_wrapper_type *probe;                                    \
        struct elem_wrapper_type wrapper;                                   \
        size_t i;                                                           \
                                                                            \
        tacc_assert(map->map->fill < map->map->cap,                         \
                    "TODO: grow hashmap: %d == %d",                         \
                    map->map->fill,                                         \
                    map->map->cap);                                         \
        i = tacc_str_hash(content->hash_key);                               \
        while (1) {                                                         \
            probe = tacc_dynhash_probe(map->map, i);                        \
            if (probe->content == NULL) {                                   \
                break;                                                      \
            }                                                               \
            i = i + 1;                                                      \
        }                                                                   \
        wrapper.content = content;                                          \
        tacc_dynhash_insert_new(map->map, i, &wrapper);                     \
    }                                                                       \
    size_t fill_count_func(struct hash_type *map) {                         \
        return tacc_dynhash_fill_count(map->map);                           \
    }                                                                       \
    void free_func(struct hash_type *map) {                                 \
        size_t i;                                                           \
        struct elem_wrapper_type *to_free;                                  \
        if (deinit_func != NULL) {                                          \
            for (i = 0; i < map->map->cap; i = i + 1) {                     \
                to_free = tacc_dynhash_probe(map->map, i);                  \
                if (to_free) {                                              \
                    deinit_func(to_free->content);                          \
                }                                                           \
            }                                                               \
        }                                                                   \
        tacc_dynhash_free(map->map);                                        \
    }

#endif
