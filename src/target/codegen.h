#ifndef TACC_TARGET_CODEGEN
#define TACC_TARGET_CODEGEN

#include "compile.h"
#include "machine.h"

struct tacc_codegen_state;
struct tacc_target_codegen_state;

struct tacc_target_codegen_state *tacc_target_codegen_state_new(void);
void tacc_target_codegen_int(struct tacc_codegen_state *state,
                             struct tacc_val *val);
void tacc_target_codegen_return_top_int(struct tacc_codegen_state *state);
void tacc_target_codegen_prelude(struct tacc_compiler *compiler);

#endif
