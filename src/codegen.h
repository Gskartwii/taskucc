#ifndef TACC_CODEGEN_H
#define TACC_CODEGEN_H

#include "dynarray.h"
#include "machine.h"
#include "statement.h"
#include "target/target.h"
#include "type.h"

enum tacc_place_kind {
    PLACE_REGISTER,
    PLACE_REGISTER_PAIR,
};

struct tacc_slot {
    enum tacc_place_kind place_kind;
    struct tacc_type *ty;
    union {
        struct tacc_target_place_register *reg;
        struct {
            struct tacc_target_place_register *reg;
            struct tacc_target_place_register *reg_2;
        } pair;
    } place;
};

DECL_DYNARRAY_OVER(tacc_slot_list,
                   tacc_slot_list_entry,
                   struct tacc_slot *,
                   tacc_slot_list_new,
                   tacc_slot_list_init,
                   tacc_slot_list_get,
                   tacc_slot_list_push,
                   tacc_slot_list_pop,
                   tacc_slot_list_len,
                   tacc_slot_list_free)

struct tacc_cg_state {
    struct tacc_function_type *func_type;
    struct tacc_target_cg_state *target_state;
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_slot_list *stack;
    struct tacc_string *code_buffer;
};

struct tacc_cg_state *tacc_cg_state_new(struct tacc_target *target,
                                        struct tacc_type_list *basic_types,
                                        struct tacc_function_type *for_type);

void tacc_cg_compile_statements(struct tacc_cg_state *state,
                                struct tacc_compound_member_list *statements);
void tacc_cg_slot_spill(struct tacc_cg_state *state, struct tacc_slot *slot);
void tacc_cg_push_reg(struct tacc_cg_state *state,
                      struct tacc_target_place_register *reg,
                      struct tacc_type *ty);
void tacc_cg_push_reg_pair(struct tacc_cg_state *state,
                           struct tacc_target_place_register *reg,
                           struct tacc_target_place_register *reg_2,
                           struct tacc_type *ty);

#ifndef __M2__
__attribute__((format(printf, 2, 3)))
#endif
void tacc_cg_output(struct tacc_cg_state *state, char *fmt, ...);

struct tacc_slot *tacc_cg_get_top(struct tacc_cg_state *state);
void tacc_cg_pop(struct tacc_cg_state *state);
void tacc_cg_state_free(struct tacc_cg_state *state);
void tacc_slot_free(struct tacc_slot *slot);

struct tacc_target_place_register *tacc_target_place_register_new(void);
void tacc_target_place_register_free(struct tacc_target_place_register *reg);
uint32_t tacc_target_cg_alloc_reg(struct tacc_cg_state *state,
                                  uint32_t desired_registers);
uint32_t tacc_target_cg_alloc_reg_pair(struct tacc_cg_state *state,
                                       uint32_t *permissible_pairs);
void tacc_cg_move_pair(struct tacc_cg_state *state,
                       struct tacc_slot *slot,
                       uint32_t permissible_low,
                       uint32_t permissible_high);
void tacc_cg_move(struct tacc_cg_state *state,
                  struct tacc_slot *slot,
                  uint32_t permissible_regs);
void tacc_cg_int_pair(struct tacc_cg_state *state, struct tacc_val *val);
void tacc_cg_ensure_top_is_pair(struct tacc_cg_state *state,
                                uint32_t *lo_reg,
                                uint32_t *hi_reg);
uint32_t tacc_cg_ensure_top_is_single(struct tacc_cg_state *state);

#endif
