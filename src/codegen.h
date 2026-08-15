#ifndef TACC_CODEGEN_H
#define TACC_CODEGEN_H

#include "dynarray.h"
#include "statement.h"
#include "target/codegen.h"
#include "target/target.h"

enum tacc_place_kind {
    PLACE_REGISTER,
    PLACE_REGISTER_PAIR,
};

struct tacc_slot {
    enum tacc_place_kind place_kind;
    struct tacc_type *ty;
    union {
        struct tacc_target_place_register *reg;
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
    struct tacc_target_cg_state *target_state;
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_slot_list *stack;
    struct tacc_string *code_buffer;
};

struct tacc_cg_state *tacc_cg_state_new(struct tacc_target *target,
                                        struct tacc_type_list *basic_types);

void tacc_cg_compile_statements(struct tacc_cg_state *state,
                                struct tacc_compound_member_list *statements);
void tacc_cg_slot_spill(struct tacc_cg_state *state, struct tacc_slot *slot);
void tacc_cg_push_reg(struct tacc_cg_state *state,
                      struct tacc_target_place_register *reg,
                      struct tacc_type *ty);
void tacc_cg_output(struct tacc_cg_state *state, char *fmt, ...);
struct tacc_slot *tacc_cg_get_top(struct tacc_cg_state *state);
void tacc_cg_pop(struct tacc_cg_state *state);
void tacc_cg_state_free(struct tacc_cg_state *state);
void tacc_slot_free(struct tacc_slot *slot);

struct tacc_target_place_register *tacc_target_place_register_new(void);
void tacc_target_place_register_free(struct tacc_target_place_register *reg);
uint32_t tacc_target_cg_alloc_reg(struct tacc_cg_state *state,
                                  uint32_t desired_registers);

#endif
