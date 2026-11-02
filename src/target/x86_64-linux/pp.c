#include "target/pp.h"

void tacc_target_pp_init_defines(struct tacc_pp_state *state) {
    tacc_pp_define(state, "__x86_64__", "1");
    tacc_pp_define(state, "__x86_64", "1");
    tacc_pp_define(state, "__amd64__", "1");
}
