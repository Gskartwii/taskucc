#ifndef TACC_TARGET_CODEGEN
#define TACC_TARGET_CODEGEN

#include "../codegen.h"
#include "compile.h"
#include "machine.h"

struct tacc_cg_state;
struct tacc_target_cg_state;

struct tacc_target_cg_state *tacc_target_cg_state_new(void);
void tacc_target_cg_int(struct tacc_cg_state *state,
                        struct tacc_val *val,
                        size_t width);
void tacc_target_cg_return_top_int(struct tacc_cg_state *state, size_t width);
void tacc_target_cg_prelude(struct tacc_compiler *compiler);
void tacc_target_cg_state_free(struct tacc_target_cg_state *state);
tacc_bool tacc_type_needs_reg_pair(struct tacc_type *ty);
void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            size_t from_width,
                            size_t to_width,
                            tacc_bool is_sext);
void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from_reg,
                                 uint32_t to_reg);
void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b);
void tacc_target_cg_finalize(struct tacc_cg_state *state);

#endif
