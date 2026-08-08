#ifndef TACC_COMPILE_H
#define TACC_COMPILE_H

#include "decl.h"
#include "target/target.h"

struct tacc_compiler {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
};

struct tacc_type *tacc_type_from_decl_type(struct tacc_compiler *compiler,
                                           struct tacc_decl_type *type);
void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl);
#endif
