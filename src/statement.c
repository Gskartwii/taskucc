#include "statement.h"
#include "util.h"

MK_DYNARRAY_OVER(tacc_statement_list,
                 tacc_statement_list_entry,
                 struct tacc_statement *,
                 tacc_statement_list_new,
                 tacc_statement_list_init,
                 tacc_statement_list_get,
                 tacc_statement_list_push,
                 tacc_statement_list_pop,
                 tacc_statement_list_len,
                 tacc_statement_free,
                 tacc_statement_list_free)

void tacc_statement_free(struct tacc_statement *statement) {
    tacc_free(statement);
}
