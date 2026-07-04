#include "dynhash.h"
#include "gcc_compat.h"
#include "util.h"
#include <memory.h>

struct tacc_dynhash *tacc_dynhash_new(size_t cap, size_t element_size) {
    struct tacc_dynhash *hash;

    hash = tacc_malloc(sizeof(struct tacc_dynhash));
    tacc_dynhash_init(hash, cap, element_size);

    return hash;
}
void tacc_dynhash_init(struct tacc_dynhash *hash,
                       size_t cap,
                       size_t element_size) {
    hash->element_size = element_size;
    hash->buffer = tacc_malloc(element_size * cap);
    hash->cap = cap;
    hash->fill = 0;
}
void *tacc_dynhash_probe(struct tacc_dynhash *hash, size_t index) {
    char *buf;

    buf = hash->buffer;

    return buf + (index & (hash->cap - 1)) * hash->element_size;
}
void tacc_dynhash_insert_new(struct tacc_dynhash *hash,
                             size_t index,
                             void *elem) {
    char *spot;
    char *buf;

    buf = hash->buffer;
    spot = buf + (index & (hash->cap - 1)) * hash->element_size;
    memcpy(spot, elem, hash->element_size);
    hash->fill = hash->fill + 1;
}
size_t tacc_dynhash_fill_count(struct tacc_dynhash *hash) { return hash->fill; }
void tacc_dynhash_free(struct tacc_dynhash *hash) {
    tacc_free(hash->buffer);
    tacc_free(hash);
}

uint32_t tacc_str_hash(char *name) {
    uint32_t h1;
    uint32_t h2;
    uint32_t ch;
    size_t i;
    size_t len;

    /* https://gist.github.com/jlevy/c246006675becc446360a798e2b2d781 */
    len = strlen(name);
    h1 = 0xdeadbeef;
    h2 = 0x41c6ce57;
    for (i = 0; i < len; i = i + 1) {
        ch = (uint32_t) (unsigned char) *name;
        h1 = (h1 ^ ch) * 2654435761U;
        h2 = (h2 ^ ch) * 1597334677U;
    }
    h1 = (h1 ^ (h1 >> 16)) * 2246822507U;
    h1 = h1 ^ ((h2 ^ (h2 >> 13)) * 3266489909U);
    return h1;
}
