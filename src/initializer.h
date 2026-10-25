#ifndef TACC_INITIALIZER_H
#define TACC_INITIALIZER_H

#include "decl.h"
#include "dynarray.h"
#include "machine.h"

struct tacc_init_command {
    struct tacc_expr *stored_expr;
    struct tacc_type *as_type;
    int offset;
};

DECL_DYNARRAY_OVER(tacc_init_command_list,
                   tacc_init_command_list_entry,
                   struct tacc_init_command *,
                   tacc_init_command_list_new,
                   tacc_init_command_list_init,
                   tacc_init_command_list_get,
                   tacc_init_command_list_push,
                   tacc_init_command_list_pop,
                   tacc_init_command_list_len,
                   tacc_init_command_list_free)

void tacc_init_command_free(struct tacc_init_command *cmd);

struct tacc_init_command_list *
tacc_init_evaluate_initializer(struct tacc_initializer *initializer,
                               struct tacc_type *initialized_type,
                               struct tacc_eval_context *eval_ctx);

#endif
