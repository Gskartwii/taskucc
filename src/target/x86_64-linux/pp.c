#include "target/pp.h"

void tacc_target_pp_init_defines(struct tacc_pp_state *state) {
    tacc_pp_define(state, "__x86_64__", "");
    tacc_pp_define(state, "__x86_64", "");
    tacc_pp_define(state, "__amd64__", "");
}
