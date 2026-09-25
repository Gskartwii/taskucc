#ifndef TACC_TARGET_CODEGEN
#define TACC_TARGET_CODEGEN

#include "../codegen.h"
#include "call_itf.h"
#include "compile.h"
#include "machine.h"

struct tacc_cg_state;
struct tacc_target_cg_state;

struct tacc_target_cg_state *tacc_target_cg_state_new(void);
void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val);
void tacc_target_cg_return_top_int(struct tacc_cg_state *state);
void tacc_target_cg_prelude(struct tacc_compiler *compiler);
void tacc_target_cg_state_free(struct tacc_target_cg_state *state);
tacc_bool tacc_type_needs_reg_pair(struct tacc_type *ty);
void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            struct tacc_type *to_type,
                            tacc_bool is_sext);
void tacc_target_cg_narrow_top(struct tacc_cg_state *state,
                               struct tacc_type *to_type,
                               tacc_bool is_sext);
void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from_reg,
                                 uint32_t to_reg);
void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b);
void tacc_target_cg_dup(struct tacc_cg_state *state);
void tacc_target_cg_addrof_var(struct tacc_cg_state *state,
                               struct tacc_local_var *var);
void tacc_target_cg_deref_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type);
void tacc_target_cg_store_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type);
void tacc_target_cg_finalize(struct tacc_cg_state *state);
void tacc_target_cg_addrof_obj(struct tacc_cg_state *state,
                               struct tacc_global_object *object);
void tacc_target_cg_alloc_stack(struct tacc_cg_state *state,
                                size_t space,
                                size_t align_p2);
void tacc_target_cg_load_scratch_part(struct tacc_cg_state *state,
                                      int offset,
                                      uint32_t to_reg,
                                      struct tacc_type *ty);
void tacc_target_cg_move_scratch_to_stack(struct tacc_cg_state *state,
                                          int from_fp_offset,
                                          int to_sp_offset,
                                          size_t size);
void tacc_target_cg_store_reg_to_scratch(struct tacc_cg_state *state,
                                         int offset,
                                         uint32_t reg,
                                         struct tacc_type *ty);
void tacc_target_cg_store_reg_pair_to_scratch(struct tacc_cg_state *state,
                                              int offset,
                                              uint32_t reg,
                                              uint32_t reg_2);
void tacc_target_cg_call_top(struct tacc_cg_state *state);
void tacc_target_cg_normalize_retval(struct tacc_cg_state *state,
                                     struct tacc_callitf *itf,
                                     struct tacc_type *return_ty);
uint32_t tacc_target_cg_reg_class_of_type(struct tacc_type *ty);

#endif
