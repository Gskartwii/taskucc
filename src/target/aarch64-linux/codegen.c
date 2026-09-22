#include "codegen.h"
#include "compile.h"
#include "machine.h"
#include "target/aarch64-linux/registers.h"
#include "target/codegen.h"
#include "type.h"
#include "util.h"

struct tacc_target_cg_state {
    uint32_t x;
};

struct tacc_target_cg_state *tacc_target_cg_state_new(void) {
    struct tacc_target_cg_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_cg_state));

    return state;
}

static char *tacc_target_register_as_64(enum tacc_target_register reg) {
    switch (reg) {
    case REG_X0:
        return "x0";
    case REG_X1:
        return "x1";
    case REG_X2:
        return "x2";
    case REG_X3:
        return "x3";
    case REG_X4:
        return "x4";
    case REG_X5:
        return "x5";
    case REG_X6:
        return "x6";
    case REG_X7:
        return "x7";
    case REG_X8:
        return "x8";
    case REG_X9:
        return "x9";
    case REG_X10:
        return "x10";
    case REG_X11:
        return "x11";
    case REG_X12:
        return "x12";
    case REG_X13:
        return "x13";
    case REG_X14:
        return "x14";
    case REG_X15:
        return "x15";
    case REG_X19:
        return "x19";
    case REG_X20:
        return "x20";
    case REG_X21:
        return "x21";
    case REG_X22:
        return "x22";
    case REG_X23:
        return "x23";
    case REG_X24:
        return "x24";
    case REG_X25:
        return "x25";
    case REG_X26:
        return "x26";
    case REG_X27:
        return "x27";
    case REG_X28:
        return "x28";
    default:
        return "INVALID-64";
    }
}

static char *tacc_target_register_as_32(enum tacc_target_register reg) {
    switch (reg) {
    case REG_X0:
        return "w0";
    case REG_X1:
        return "w1";
    case REG_X2:
        return "w2";
    case REG_X3:
        return "w3";
    case REG_X4:
        return "w4";
    case REG_X5:
        return "w5";
    case REG_X6:
        return "w6";
    case REG_X7:
        return "w7";
    case REG_X8:
        return "w8";
    case REG_X9:
        return "w9";
    case REG_X10:
        return "w10";
    case REG_X11:
        return "w11";
    case REG_X12:
        return "w12";
    case REG_X13:
        return "w13";
    case REG_X14:
        return "w14";
    case REG_X15:
        return "w15";
    case REG_X19:
        return "w19";
    case REG_X20:
        return "w20";
    case REG_X21:
        return "w21";
    case REG_X22:
        return "w22";
    case REG_X23:
        return "w23";
    case REG_X24:
        return "w24";
    case REG_X25:
        return "w25";
    case REG_X26:
        return "w26";
    case REG_X27:
        return "w27";
    case REG_X28:
        return "w28";
    default:
        return "INVALID-32";
    }
}

static char *tacc_target_register_name(enum tacc_target_register reg,
                                       size_t width) {
    switch (width) {
    case 64:
        return tacc_target_register_as_64(reg);
    case 32:
    case 16:
    case 8:
        return tacc_target_register_as_32(reg);
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid width %d", width);
        return NULL;
    }
}

void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;
    size_t width;

    register_place = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
    width = tacc_type_bit_width(val->type);
    if (width <= 32) {
        reg = tacc_target_register_as_32(register_place);
    } else {
        reg = tacc_target_register_as_64(register_place);
    }
    tacc_cg_output(
        state, "\n\t movz %s, #0x%x", reg, val->value.int_value->low & 0xFFFF);
    if ((val->value.int_value->low >> ((unsigned) 16)) != 0) {
        tacc_cg_output(state,
                       "\n\t movk %s, #0x%x, lsl #16",
                       reg,
                       (val->value.int_value->low >> ((unsigned) 16)) & 0xFFFF);
    }
    if (width > 32) {
        if ((val->value.int_value->high & 0xFFFF) != 0) {
            tacc_cg_output(state,
                           "\n\t movk %s, #0x%x, lsl #32",
                           reg,
                           (val->value.int_value->high) & 0xFFFF);
        }
        if ((val->value.int_value->high >> ((unsigned) 16)) != 0) {
            tacc_cg_output(state,
                           "\n\t movk %s, #0x%x, lsl #48",
                           reg,
                           (val->value.int_value->high >> ((unsigned) 16)) &
                               0xFFFF);
        }
    }

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_cg_push_reg(state, reg_place, val->type);
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    tacc_cg_move(state, tacc_cg_get_top(state), REG_X0);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t b .L%u_epilog", state->func_name);
}

void tacc_target_cg_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

void tacc_target_cg_state_free(struct tacc_target_cg_state *state) {
    tacc_free(state);
}

tacc_bool tacc_type_needs_reg_pair(struct tacc_type *ty) {
    TACC_UNUSED(ty);

    return 0;
}

void tacc_target_cg_narrow_top(struct tacc_cg_state *state,
                               struct tacc_type *to_type,
                               tacc_bool is_sext) {
    tacc_target_cg_ext_top(state, to_type, is_sext);
}

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            struct tacc_type *type,
                            tacc_bool is_sext) {
    struct tacc_slot *slot;
    enum tacc_target_register top_reg;
    char *reg_name;
    int width;
    size_t from_width;
    size_t to_width;

    slot = tacc_cg_get_top(state);
    tacc_cg_move(state, slot, REG_VOLATILE);
    top_reg = slot->place.reg->reg;
    from_width = tacc_type_bit_width(slot->ty);
    to_width = tacc_type_bit_width(type);

    if (from_width == to_width) {
        /*
         * Plain sign-conversion. For widths < 32, the value is already
         * sign-extended. For widths 32 and 64, there is no sign/zero extension
         * to be done, as the register width encodes the width.
         */
        return;
    }
    if (from_width > to_width) {
        /* narrowing path */
        width = (int) to_width;
    } else {
        width = (int) from_width;
    }

    if (to_width <= 32) {
        reg_name = tacc_target_register_as_32(top_reg);
    } else {
        reg_name = tacc_target_register_as_64(top_reg);
    }
    if (is_sext) {
        tacc_cg_output(
            state, "\n\t sbfx %s, %s, #0, #%d", reg_name, reg_name, width);
    } else {
        tacc_cg_output(
            state, "\n\t ubfx %s, %s, #0, #%d", reg_name, reg_name, width);
    }
}

void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from,
                                 uint32_t to) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(from, 64);
    reg_name_2 = tacc_target_register_name(to, 64);
    tacc_cg_output(state, "\n\t mov %s, %s", reg_name_2, reg_name);
}

void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(reg_a, 64);
    reg_name_2 = tacc_target_register_name(reg_b, 64);

    tacc_cg_output(
        state, "\n\t eor %s, %s, %s", reg_name, reg_name, reg_name_2);
    tacc_cg_output(
        state, "\n\t eor %s, %s, %s", reg_name_2, reg_name, reg_name);
    tacc_cg_output(
        state, "\n\t eor %s, %s, %s", reg_name, reg_name, reg_name_2);
}

static void tacc_target_cg_store(struct tacc_cg_state *state,
                                 uint32_t reg,
                                 int off,
                                 struct tacc_type *lval_ty,
                                 tacc_bool in_prelude) {
    char *src_reg;
    char *width_suffix;

    src_reg = tacc_target_register_name(reg, tacc_type_bit_width(lval_ty));
    width_suffix = "";
    if (tacc_type_bit_width(lval_ty) == 16) {
        width_suffix = "h";
    }
    if (tacc_type_bit_width(lval_ty) == 8) {
        width_suffix = "b";
    }

    if (in_prelude) {
        tacc_cg_output_prelude(
            state, "\n\t str%s %s, [fp, %d]", width_suffix, src_reg, off);
    } else {
        tacc_cg_output(
            state, "\n\t str%s %s, [fp, %d]", width_suffix, src_reg, off);
    }
}

static void tacc_target_cg_copy_param(struct tacc_cg_state *state,
                                      struct tacc_callitf_part *in_place,
                                      struct tacc_local_var *locvar_place) {
    switch (in_place->place.kind) {
    case CALLITF_PLACE_REGISTER:
        tacc_assert(ASSERT_TODO,
                    in_place->place.extra.reg.reg_class <= REGC_INT_X,
                    "non-integral function parameters");
        tacc_target_cg_store(state,
                             in_place->place.extra.reg.reg,
                             (int) (in_place->offset_from_param_start) +
                                 locvar_place->offset,
                             locvar_place->ty,
                             1);
        break;
    case CALLITF_PLACE_REGISTER_PAIR:
        tacc_assert(ASSERT_ICE,
                    0,
                    "ICE: didn't expect a register pair param on aarch64");
        break;
    case CALLITF_PLACE_STACK:
        /* skip */
        break;
    }
}
void tacc_target_cg_finalize(struct tacc_cg_state *state) {
    struct tacc_callitf_part_list_entry *param_entry;
    struct tacc_ident_list_entry *param_ident_entry;
    struct tacc_local_var_map_entry *locvar_place;
    size_t i;

    tacc_cg_output_prelude(state, "\n\t stp fp, lr, [sp, #-16]");
    /* non-volatile registers not allocated */
    tacc_cg_output_prelude(state,
                           "\n\t sub sp, sp, %d",
                           ((int) (state->num_local_bytes + 0xF) & ~0xF) + 16);
    tacc_cg_output_prelude(state, "\n\t mov fp, sp");

    for (i = 0; i < tacc_callitf_part_list_len(state->interface->param_parts);
         i = i + 1) {
        param_entry =
            tacc_callitf_part_list_get(state->interface->param_parts, i);
        param_ident_entry = tacc_ident_list_get(state->param_names, i);
        locvar_place =
            tacc_local_var_map_get(state->locals, param_ident_entry->content);
        tacc_target_cg_copy_param(
            state, param_entry->content, locvar_place->content);
    }

    tacc_cg_output(state, "\n\t mov sp, fp");
    tacc_cg_output(state,
                   "\n\t add sp, sp, %d",
                   ((int) (state->num_local_bytes + 0xF) & ~0xF) + 16);
    tacc_cg_output(state, "\n\t ldp fp, lr, [sp, #-16]");
    tacc_cg_output(state, "\n\t ret");
}

void tacc_target_cg_deref_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    struct tacc_slot *slot;
    size_t load_width;
    uint32_t reg;
    char *reg_name;
    char *addr_name;
    char *sext;

    slot = tacc_cg_get_top(state);
    load_width = tacc_type_bit_width(int_type);
    reg = tacc_cg_ensure_top_is_single(state);
    addr_name = tacc_target_register_as_64(reg);
    if (load_width > 32) {
        reg_name = tacc_target_register_as_64(reg);
    } else {
        reg_name = tacc_target_register_as_32(reg);
    }
    sext = "";
    if (tacc_type_kind_is_signed(int_type->kind)) {
        sext = "s";
    }

    switch (load_width) {
    case 8:
        tacc_cg_output(
            state, "\n\t ldr%sb %s, [%s]", sext, reg_name, addr_name);
        break;
    case 16:
        tacc_cg_output(
            state, "\n\t ldr%sh %s, [%s]", sext, reg_name, addr_name);
        break;
    case 32:
    case 64:
        tacc_cg_output(state, "\n\t ldr %s, [%s]", reg_name, addr_name);
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid load width %d", load_width);
    }

    slot->ty = int_type;
}

void tacc_target_cg_store_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    size_t store_width;
    uint32_t reg;
    uint32_t addr_reg;
    char *reg_name;
    char *addr_name;

    store_width = tacc_type_bit_width(int_type);
    reg = tacc_cg_ensure_top_is_single(state);
    addr_reg = tacc_cg_ensure_over_is_single(state);
    addr_name = tacc_target_register_as_64(addr_reg);
    if (store_width > 32) {
        reg_name = tacc_target_register_as_64(reg);
    } else {
        reg_name = tacc_target_register_as_32(reg);
    }

    switch (store_width) {
    case 8:
        tacc_cg_output(state, "\n\t strb %s, [%s]", reg_name, addr_name);
        break;
    case 16:
        tacc_cg_output(state, "\n\t strh %s, [%s]", reg_name, addr_name);
        break;
    case 32:
    case 64:
        tacc_cg_output(state, "\n\t str %s, [%s]", reg_name, addr_name);
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid store width %d", store_width);
    }

    tacc_cg_pop(state);
    tacc_cg_pop(state);
}

void tacc_target_cg_addrof_var(struct tacc_cg_state *state,
                               struct tacc_local_var *var) {
    struct tacc_target_place_register *reg_place;
    uint32_t reg;
    char *reg_name;

    reg = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
    reg_place = tacc_target_place_register_new();
    reg_place->reg = reg;

    reg_name = tacc_target_register_as_64(reg);
    tacc_cg_output(state, "\n\t add %s, fp, #%d", reg_name, var->offset);
    tacc_cg_push_reg(
        state,
        reg_place,
        tacc_type_to_pointer(state->compiler->target->pointer_ty, var->ty, 1));
}

void tacc_target_cg_dup(struct tacc_cg_state *state) {
    struct tacc_slot *slot;
    struct tacc_target_place_register *reg_place;
    uint32_t reg;
    uint32_t new_reg;

    slot = tacc_cg_get_top(state);
    reg = tacc_cg_ensure_top_is_single(state);
    new_reg = tacc_target_cg_alloc_reg(state, REG_VOLATILE & ~reg);
    tacc_target_cg_move_reg_reg(state, reg, new_reg);
    reg_place = tacc_target_place_register_new();
    reg_place->reg = new_reg;
    tacc_cg_push_reg(state, reg_place, slot->ty);
}

void tacc_target_cg_addrof_obj(struct tacc_cg_state *state,
                               struct tacc_global_object *object) {
    uint32_t reg;
    struct tacc_target_place_register *reg_place;
    char *reg_name;
    struct tacc_string *symbol_name;

    reg = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
    reg_name = tacc_target_register_as_64(reg);
    symbol_name = tacc_compile_get_name(state->compiler, object->name_ref);
    tacc_cg_output(state,
                   "\n\t adrp %s, %s",
                   reg_name,
                   tacc_dynstring_as_str(symbol_name));
    tacc_cg_output(state,
                   "\n\t add %s, %s, :lo12:%s",
                   reg_name,
                   reg_name,
                   tacc_dynstring_as_str(symbol_name));
    reg_place = tacc_target_place_register_new();
    reg_place->reg = reg;
    tacc_cg_push_reg(state,
                     reg_place,
                     tacc_type_to_pointer(state->compiler->target->pointer_ty,
                                          object->extra.obj_type,
                                          1));
}
