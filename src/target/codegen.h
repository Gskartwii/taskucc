#ifndef TACC_TARGET_CODEGEN
#define TACC_TARGET_CODEGEN

#include "compile.h"
#include "machine.h"

struct tacc_cg_state;
struct tacc_target_cg_state;

struct tacc_target_cg_state *tacc_target_cg_state_new(void);
void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val);
void tacc_target_cg_return_top_int(struct tacc_cg_state *state);
void tacc_target_cg_prelude(struct tacc_compiler *compiler);
void tacc_target_cg_state_free(struct tacc_target_cg_state *state);

#endif
