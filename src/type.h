#ifndef TACC_TYPE_H
#define TACC_TYPE_H

#include "dynarray.h"
#include "dynhash.h"
#include "dynstring.h"
#include "target/target.h"
#include "util.h"

enum tacc_type_kind {
    TYK_UCHAR,
    TYK_SCHAR,
    TYK_USHORT,
    TYK_SSHORT,
    TYK_UINT,
    TYK_SINT,
    TYK_ULONG,
    TYK_SLONG,
    TYK_ULONGLONG,
    TYK_SLONGLONG,
    TYK_FLOAT,
    TYK_DOUBLE,
    TYK_LONGDOUBLE,

    TYK_BOOL,
    TYK_VOID,

    TYK_PTR,
    TYK_STRUCT,
    TYK_UNION,
    TYK_ENUM,
    TYK_ARRAY,
    TYK_INCOMPLETE_ARRAY,
    TYK_VLA,
    TYK_DECAYING_VLA,
    TYK_FN
};

enum tacc_int_rank {
    IRANK_BOOL,
    IRANK_CHAR,
    IRANK_SHORT,
    IRANK_INT,
    IRANK_LONG,
    IRANK_LLONG
};

struct tacc_function_type {
    tacc_bool is_vararg;

    /* list: owning, content: borrow */
    struct tacc_type_list *param_types;

    /* borrow */
    struct tacc_type *return_type;
};

struct tacc_array_type {
    /* borrow */
    struct tacc_type *element_type;

    /* owning */
    struct tacc_u64 *dimension;
};

struct tacc_enumeration_type {
    /* borrow */
    /*
     * For strictly conforming C99, in practice this should be either TYK_SINT
     * or TYK_UINT, for GCC compatibility. As a GNU extension, GCC also permits
     * larger-sized integer types, with a warning on -Wpedantic.
     *
     * Expressions of enumerators can have a different type than the
     * enumeration's representation type. This gives the type used for storing
     * and passing values declared with the type of the enumeration (the
     * representation type).
     */
    struct tacc_type *underlying_type;

    /* TODO: should we define enumerators here? */
};

struct tacc_field {
    size_t offset;
    tacc_bool is_bitfield;

    size_t bit_offset;
    size_t bit_width;

    /* borrow */
    struct tacc_type *type;

    /* owning */
    struct tacc_string *name;
};

DECL_DYNARRAY_OVER(tacc_field_list,
                   tacc_field_list_entry,
                   struct tacc_field *,
                   tacc_field_list_new,
                   tacc_field_list_init,
                   tacc_field_list_get,
                   tacc_field_list_push,
                   tacc_field_list_pop,
                   tacc_field_list_len,
                   tacc_field_list_free)

struct tacc_struct_type {
    size_t alignment_p2;
    size_t size;

    /* owning */
    struct tacc_field_list *fields;
};

struct tacc_union_type {
    size_t alignment_p2;
    size_t size;

    /* owning */
    struct tacc_field_list *fields;
};

struct tacc_type {
    enum tacc_type_kind kind;

    union {
        /* owning */
        struct tacc_function_type *function;

        /* owning */
        struct tacc_array_type *array;

        /* owning */
        struct tacc_enumeration_type *enumeration;

        /* owning */
        struct tacc_struct_type *structure;

        /* owning */
        struct tacc_union_type *onion;

        /* borrow */
        struct {
            struct tacc_type *pointee;
            struct tacc_ptr_type *repr;
        } pointer;

        /* borrow */
        struct tacc_int_type *int_repr;
    } extra;

    /* owning */
    struct tacc_string *name;

    /* owning */
    struct tacc_type *derived_ptr;

    /* owning */
    struct tacc_type_list *derived_array_types;

    /* owning */
    struct tacc_type_list *derived_func_types;
};

enum tacc_conversion_kind {
    CONV_NONE,
    CONV_LEFT,
    CONV_RIGHT,
    CONV_BOTH,
};

DECL_DYNARRAY_OVER(tacc_type_list,
                   tacc_type_list_entry,
                   struct tacc_type *,
                   tacc_type_list_new,
                   tacc_type_list_init,
                   tacc_type_list_get,
                   tacc_type_list_push,
                   tacc_type_list_pop,
                   tacc_type_list_len,
                   tacc_type_list_free)
DECL_DYNHASH_OVER(tacc_type_map,
                  tacc_type_map_entry,
                  struct tacc_type *,
                  tacc_type_map_new,
                  tacc_type_map_init,
                  tacc_type_map_get,
                  tacc_type_map_insert,
                  tacc_type_map_count,
                  tacc_type_map_free)

struct tacc_type *tacc_type_new(void);
struct tacc_array_type *tacc_array_type_new(void);
struct tacc_function_type *tacc_function_type_new(void);
struct tacc_enumeration_type *tacc_enumeration_type_new(void);
struct tacc_struct_type *tacc_struct_type_new(void);
struct tacc_union_type *tacc_union_type_new(void);
struct tacc_field *tacc_field_new(void);
struct tacc_type *tacc_get_basic_type(struct tacc_type_list *basic_types,
                                      enum tacc_type_kind kind);
struct tacc_type *tacc_type_to_pointer(struct tacc_ptr_type *repr,
                                       struct tacc_type *base_type,
                                       size_t indirection_level);
tacc_bool tacc_type_kind_is_signed(enum tacc_type_kind kind);
tacc_bool tacc_type_kind_is_scalar(enum tacc_type_kind type_kind);
tacc_bool tacc_type_is_scalar(struct tacc_type *type);
tacc_bool tacc_type_is_subset(struct tacc_type *subset,
                              struct tacc_type *superset);
tacc_bool tacc_type_is_integral(struct tacc_type *type);
tacc_bool tacc_type_kind_is_integral(enum tacc_type_kind type_kind);
size_t tacc_type_bit_width(struct tacc_type *type);
size_t tacc_type_alignment_p2(struct tacc_type *type);
size_t tacc_type_size(struct tacc_type *type);
enum tacc_type_kind
tacc_type_usual_arithmetic_conversions(enum tacc_conversion_kind *kind_out,
                                       struct tacc_type *left,
                                       struct tacc_type *right);
enum tacc_int_rank tacc_type_rank(enum tacc_type_kind kind);
enum tacc_type_kind tacc_type_to_unsigned(enum tacc_type_kind kind);
void tacc_gen_basic_types(struct tacc_target *target,
                          struct tacc_type_list *into);
void tacc_type_free(struct tacc_type *type);
void tacc_array_type_free(struct tacc_array_type *type);
void tacc_function_type_free(struct tacc_function_type *type);
void tacc_enumeration_type_free(struct tacc_enumeration_type *type);
void tacc_struct_type_free(struct tacc_struct_type *type);
void tacc_union_type_free(struct tacc_union_type *type);
void tacc_field_free(struct tacc_field *field);

#endif
