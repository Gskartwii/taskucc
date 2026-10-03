#include "codegen.h"
#include "call_itf.h"
#include "compile.h"
#include "machine.h"
#include "target/aarch64-linux/registers.h"
#include "target/codegen.h"
#include "target/target.h"
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

static char *tacc_target_f_register_as_32(enum tacc_target_register_float reg) {
    switch (reg) {
    case REGF_V0:
        return "s0";
    case REGF_V1:
        return "s1";
    case REGF_V2:
        return "s2";
    case REGF_V3:
        return "s3";
    case REGF_V4:
        return "s4";
    case REGF_V5:
        return "s5";
    case REGF_V6:
        return "s6";
    case REGF_V7:
        return "s7";
    case REGF_V8:
        return "s8";
    case REGF_V9:
        return "s9";
    case REGF_V10:
        return "s10";
    case REGF_V11:
        return "s11";
    case REGF_V12:
        return "s12";
    case REGF_V13:
        return "s13";
    case REGF_V14:
        return "s14";
    case REGF_V15:
        return "s15";
    case REGF_V16:
        return "s16";
    case REGF_V17:
        return "s17";
    case REGF_V18:
        return "s18";
    case REGF_V19:
        return "s19";
    case REGF_V20:
        return "s20";
    case REGF_V21:
        return "s21";
    case REGF_V22:
        return "s22";
    case REGF_V23:
        return "s23";
    case REGF_V24:
        return "s24";
    case REGF_V25:
        return "s25";
    case REGF_V26:
        return "s26";
    case REGF_V27:
        return "s27";
    case REGF_V28:
        return "s28";
    case REGF_V29:
        return "s29";
    case REGF_V30:
        return "s30";
    default:
        return "INVALID-F32";
    }
}
static char *tacc_target_f_register_as_64(enum tacc_target_register_float reg) {
    switch (reg) {
    case REGF_V0:
        return "d0";
    case REGF_V1:
        return "d1";
    case REGF_V2:
        return "d2";
    case REGF_V3:
        return "d3";
    case REGF_V4:
        return "d4";
    case REGF_V5:
        return "d5";
    case REGF_V6:
        return "d6";
    case REGF_V7:
        return "d7";
    case REGF_V8:
        return "d8";
    case REGF_V9:
        return "d9";
    case REGF_V10:
        return "d10";
    case REGF_V11:
        return "d11";
    case REGF_V12:
        return "d12";
    case REGF_V13:
        return "d13";
    case REGF_V14:
        return "d14";
    case REGF_V15:
        return "d15";
    case REGF_V16:
        return "d16";
    case REGF_V17:
        return "d17";
    case REGF_V18:
        return "d18";
    case REGF_V19:
        return "d19";
    case REGF_V20:
        return "d20";
    case REGF_V21:
        return "d21";
    case REGF_V22:
        return "d22";
    case REGF_V23:
        return "d23";
    case REGF_V24:
        return "d24";
    case REGF_V25:
        return "d25";
    case REGF_V26:
        return "d26";
    case REGF_V27:
        return "d27";
    case REGF_V28:
        return "d28";
    case REGF_V29:
        return "d29";
    case REGF_V30:
        return "d30";
    default:
        return "INVALID-F64";
    }
}

static char *
tacc_target_f_register_as_128(enum tacc_target_register_float reg) {
    switch (reg) {
    case REGF_V0:
        return "q0";
    case REGF_V1:
        return "q1";
    case REGF_V2:
        return "q2";
    case REGF_V3:
        return "q3";
    case REGF_V4:
        return "q4";
    case REGF_V5:
        return "q5";
    case REGF_V6:
        return "q6";
    case REGF_V7:
        return "q7";
    case REGF_V8:
        return "q8";
    case REGF_V9:
        return "q9";
    case REGF_V10:
        return "q10";
    case REGF_V11:
        return "q11";
    case REGF_V12:
        return "q12";
    case REGF_V13:
        return "q13";
    case REGF_V14:
        return "q14";
    case REGF_V15:
        return "q15";
    case REGF_V16:
        return "q16";
    case REGF_V17:
        return "q17";
    case REGF_V18:
        return "q18";
    case REGF_V19:
        return "q19";
    case REGF_V20:
        return "q20";
    case REGF_V21:
        return "q21";
    case REGF_V22:
        return "q22";
    case REGF_V23:
        return "q23";
    case REGF_V24:
        return "q24";
    case REGF_V25:
        return "q25";
    case REGF_V26:
        return "q26";
    case REGF_V27:
        return "q27";
    case REGF_V28:
        return "q28";
    case REGF_V29:
        return "q29";
    case REGF_V30:
        return "q30";
    default:
        return "INVALID-F128";
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

static char *tacc_target_f_register_name(enum tacc_target_register_float reg,
                                         size_t width) {
    switch (width) {
    case 128:
        return tacc_target_f_register_as_128(reg);
    case 64:
        return tacc_target_f_register_as_64(reg);
    case 32:
        return tacc_target_f_register_as_32(reg);
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid float width %d", width);
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

void tacc_target_cg_adjust_top_for_return(struct tacc_cg_state *state,
                                          struct tacc_callitf *itf) {
    int offset;
    if (itf->retval_reg_class == REGC_FLOAT_Q) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state,
                       "\n\t ldr %s, [fp, #%d]",
                       tacc_target_f_register_as_128(itf->retval_reg),
                       offset);
        return;
    }
    tacc_assert(ASSERT_ICE, 0, "unknown retval reg class");
}

void tacc_target_cg_jump_to_return(struct tacc_cg_state *state) {
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
void tacc_target_cg_move_f_reg_reg(struct tacc_cg_state *state,
                                   uint32_t from,
                                   uint32_t to) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_f_register_name(from, 64);
    reg_name_2 = tacc_target_f_register_name(to, 64);
    tacc_cg_output(state, "\n\t fmov %s, %s", reg_name_2, reg_name);
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
    reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
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
    reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
    addr_reg = tacc_cg_ensure_over_is_single(state, REG_VOLATILE & ~reg);
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
    reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
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
void tacc_target_cg_alloc_stack(struct tacc_cg_state *state,
                                size_t space,
                                size_t align_p2) {
    if (align_p2 > 3) {
        tacc_cg_output(state, "\n\t and sp, sp, #-%d", 1 << align_p2);
    }
#ifdef __M2__
    tacc_cg_output(state, "\n\t sub sp, sp, #%d", space);
#else
    tacc_cg_output(state, "\n\t sub sp, sp, #%" PRIsz "", space);
#endif
}

void tacc_target_cg_load_scratch_part(struct tacc_cg_state *state,
                                      int offset,
                                      uint32_t to_reg,
                                      struct tacc_type *ty) {
    tacc_cg_output(state,
                   "\n\t ldr %s, [fp, #%d]",
                   tacc_target_register_name(to_reg, tacc_type_bit_width(ty)),
                   offset);
}

void tacc_target_cg_move_scratch_to_stack(struct tacc_cg_state *state,
                                          int from_fp_offset,
                                          int to_sp_offset,
                                          size_t size) {
    size_t i;

    tacc_assert(ASSERT_ICE,
                (size & 7) == 0,
                "move_scratch_to_stack: not a multiple of 8");
    for (i = 0; i < size; i = i + 8) {
        tacc_cg_output(
            state, "\n\t ldr x9, [fp, #%d]", from_fp_offset + (int) i);
        tacc_cg_output(state, "\n\t str x9, [sp, #%d]", to_sp_offset + (int) i);
    }
}

void tacc_target_cg_store_reg_to_scratch(struct tacc_cg_state *state,
                                         int offset,
                                         uint32_t reg,
                                         struct tacc_type *ty) {
    tacc_cg_output(state,
                   "\n\t str %s, [fp, #%d]",
                   tacc_target_register_name(reg, tacc_type_bit_width(ty)),
                   offset);
}

void tacc_target_cg_store_reg_pair_to_scratch(struct tacc_cg_state *state,
                                              int offset,
                                              uint32_t reg,
                                              uint32_t reg_2) {
    TACC_UNUSED(state);
    TACC_UNUSED(offset);
    TACC_UNUSED(reg);
    TACC_UNUSED(reg_2);
    tacc_assert(ASSERT_ICE, 0, "not expecting to spill register pairs");
}

void tacc_target_cg_call_top(struct tacc_cg_state *state) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    tacc_assert(ASSERT_ICE,
                slot->place_kind == PLACE_SCRATCH,
                "expected callee pointer to be in scratch");
    tacc_cg_output(state, "\n\t ldr x9, [fp, #%d]", slot->place.offset);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t blr x9");
}

void tacc_target_cg_normalize_retval(struct tacc_cg_state *state,
                                     struct tacc_callitf *itf,
                                     struct tacc_type *return_ty) {
    struct tacc_target_place_register *reg_place;

    switch (itf->retval_kind) {
    case CALLITF_RETVAL_NONE:
        tacc_cg_push_void(state);
        break;
    case CALLITF_RETVAL_REGISTER:
        tacc_assert(ASSERT_TODO,
                    itf->retval_reg_class <= REGC_INT_X,
                    "returning non-integer register");
        reg_place = tacc_target_place_register_new();
        reg_place->reg = itf->retval_reg;
        tacc_cg_push_reg(state, reg_place, return_ty);
        break;
    case CALLITF_RETVAL_REGISTER_PAIR:
        tacc_assert(ASSERT_TODO, 0, "return of regpair");
        break;
    case CALLITF_RETVAL_OUTPARAM:
        tacc_assert(ASSERT_TODO, 0, "outparam returns");
        break;
    }
}

void tacc_target_cg_convert_float(struct tacc_cg_state *state,
                                  struct tacc_type *to_type) {
    uint32_t reg;
    struct tacc_slot *slot;
    struct tacc_type *ty;

    slot = tacc_cg_get_top(state);
    if (tacc_type_is_compatible(slot->ty, to_type)) {
        return;
    }
    if (slot->ty->kind == TYK_LONGDOUBLE) {
        if (to_type->kind == TYK_FLOAT) {
            ty = tacc_cg_push_func(state, PREDEF__TACCRT_TRUNCTFSF2);
        } else {
            ty = tacc_cg_push_func(state, PREDEF__TACCRT_TRUNCTFDF2);
        }
        slot = tacc_cg_get_top(state);
        tacc_cg_swap(state);
        tacc_cg_call(state, ty, 1);
        return;
    }
    if (to_type->kind == TYK_LONGDOUBLE) {
        if (slot->ty->kind == TYK_FLOAT) {
            ty = tacc_cg_push_func(state, PREDEF__TACCRT_EXTENDSFTF2);
        } else {
            ty = tacc_cg_push_func(state, PREDEF__TACCRT_EXTENDDFTF2);
        }
        slot = tacc_cg_get_top(state);
        tacc_cg_swap(state);
        tacc_cg_call(state, ty, 1);
        return;
    }
    reg = tacc_cg_ensure_top_is_single_f(state, REGV_VOLATILE);
    if (to_type->kind == TYK_FLOAT) {
        tacc_cg_output(state,
                       "\n\t fcvt %s, %s",
                       tacc_target_f_register_name(reg, 32),
                       tacc_target_f_register_name(reg, 64));
    } else {
        tacc_cg_output(state,
                       "\n\t fcvt %s, %s",
                       tacc_target_f_register_name(reg, 64),
                       tacc_target_f_register_name(reg, 32));
    }
}

void tacc_target_cg_deref_float(struct tacc_cg_state *state,
                                struct tacc_type *float_type) {
    int offset;
    uint32_t ptr_reg;
    uint32_t reg;
    struct tacc_target_place_register *reg_place;

    ptr_reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
    reg = tacc_target_cg_alloc_freg(state, REGV_VOLATILE);
    if (float_type->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_alloc_scratch(state,
                                       tacc_type_size(float_type),
                                       tacc_type_alignment_p2(float_type));
        tacc_cg_output(state,
                       "\n\t ldr %s, [%s]",
                       tacc_target_f_register_as_128(reg),
                       tacc_target_register_as_64(ptr_reg));
        tacc_cg_output(state,
                       "\n\t str %s, [fp, #%d]",
                       tacc_target_f_register_as_128(reg),
                       offset);
        tacc_cg_push_scratch(state, offset, float_type);
        return;
    }
    if (float_type->kind == TYK_DOUBLE) {
        tacc_cg_output(state,
                       "\n\t ldr %s, [%s]",
                       tacc_target_f_register_as_64(reg),
                       tacc_target_register_as_64(ptr_reg));
    } else {
        tacc_cg_output(state,
                       "\n\t ldr %s, [%s]",
                       tacc_target_f_register_as_32(reg),
                       tacc_target_register_as_64(ptr_reg));
    }
    reg_place = tacc_target_place_register_new();
    reg_place->reg = reg;
    tacc_cg_pop(state);
    tacc_cg_push_freg(state, reg_place, float_type);
}

void tacc_target_cg_store_float(struct tacc_cg_state *state,
                                struct tacc_type *float_type) {
    int offset;
    uint32_t ptr_reg;
    uint32_t reg;

    ptr_reg = tacc_cg_ensure_over_is_single(state, REG_VOLATILE);
    reg = tacc_target_cg_alloc_freg(state, REGV_VOLATILE);
    if (float_type->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state,
                       "\n\t ldr %s, [fp, #%d]",
                       tacc_target_f_register_as_128(reg),
                       offset);
        tacc_cg_output(state,
                       "\n\t str %s, [%s]",
                       tacc_target_f_register_as_128(reg),
                       tacc_target_register_as_64(ptr_reg));
        tacc_cg_pop(state);
        tacc_cg_pop(state);
        return;
    }
    if (float_type->kind == TYK_DOUBLE) {
        tacc_cg_output(state,
                       "\n\t str %s, [%s]",
                       tacc_target_f_register_as_64(reg),
                       tacc_target_register_as_64(ptr_reg));
    } else {
        tacc_cg_output(state,
                       "\n\t str %s, [%s]",
                       tacc_target_f_register_as_32(reg),
                       tacc_target_register_as_64(ptr_reg));
    }
    tacc_cg_pop(state);
    tacc_cg_pop(state);
}
