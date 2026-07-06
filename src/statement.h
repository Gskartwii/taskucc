#ifndef TACC_STATEMENT_H
#define TACC_STATEMENT_H

#include "dynarray.h"

struct tacc_statement {
    int x; /* TODO */
};

void tacc_statement_free(struct tacc_statement *statement);

DECL_DYNARRAY_OVER(tacc_statement_list,
                   tacc_statement_list_entry,
                   struct tacc_statement *,
                   tacc_statement_list_new,
                   tacc_statement_list_init,
                   tacc_statement_list_get,
                   tacc_statement_list_push,
                   tacc_statement_list_pop,
                   tacc_statement_list_len,
                   tacc_statement_list_free)

#endif
