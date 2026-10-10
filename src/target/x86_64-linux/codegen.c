#include "codegen.h"
#include "call_itf.h"
#include "compile.h"
#include "expr.h"
#include "machine.h"
#include "target/codegen.h"
#include "target/target.h"
#include "target/x86_64-linux/registers.h"
#include "type.h"
#include "util.h"

struct tacc_target_cg_state {
    int x;
};

struct tacc_target_cg_state *tacc_target_cg_state_new(void) {
    struct tacc_target_cg_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_cg_state));

    return state;
}

static char *tacc_target_register_as_64(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%rax";
    case REG_RBX:
        return "%rbx";
    case REG_RCX:
        return "%rcx";
    case REG_RDX:
        return "%rdx";
    case REG_RSI:
        return "%rsi";
    case REG_RDI:
        return "%rdi";
    case REG_R8:
        return "%r8";
    case REG_R9:
        return "%r9";
    case REG_R10:
        return "%r10";
    case REG_R11:
        return "%r11";
    case REG_R12:
        return "%r12";
    case REG_R13:
        return "%r13";
    case REG_R14:
        return "%r14";
    case REG_R15:
        return "%r15";
    default:
        return "INVALID-64";
    }
}

static char *tacc_target_register_as_32(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%eax";
    case REG_RBX:
        return "%ebx";
    case REG_RCX:
        return "%ecx";
    case REG_RDX:
        return "%edx";
    case REG_RSI:
        return "%esi";
    case REG_RDI:
        return "%edi";
    case REG_R8:
        return "%r8d";
    case REG_R9:
        return "%r9d";
    case REG_R10:
        return "%r10d";
    case REG_R11:
        return "%r11d";
    case REG_R12:
        return "%r12d";
    case REG_R13:
        return "%r13d";
    case REG_R14:
        return "%r14d";
    case REG_R15:
        return "%r15d";
    default:
        return "INVALID-32";
    }
}

static char *tacc_target_register_as_xmm(enum tacc_target_register_float reg) {
    switch (reg) {
    case REGV_XMM0:
        return "%xmm0";
    case REGV_XMM1:
        return "%xmm1";
    case REGV_XMM2:
        return "%xmm2";
    case REGV_XMM3:
        return "%xmm3";
    case REGV_XMM4:
        return "%xmm4";
    case REGV_XMM5:
        return "%xmm5";
    case REGV_XMM6:
        return "%xmm6";
    case REGV_XMM7:
        return "%xmm7";
    case REGV_XMM8:
        return "%xmm8";
    case REGV_XMM9:
        return "%xmm9";
    case REGV_XMM10:
        return "%xmm10";
    case REGV_XMM11:
        return "%xmm11";
    case REGV_XMM12:
        return "%xmm12";
    case REGV_XMM13:
        return "%xmm13";
    case REGV_XMM14:
        return "%xmm14";
    case REGV_XMM15:
        return "%xmm15";
    default:
        return "INVALID-XMM";
    }
}

static char *tacc_target_register_as_16(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%ax";
    case REG_RBX:
        return "%bx";
    case REG_RCX:
        return "%cx";
    case REG_RDX:
        return "%dx";
    case REG_RSI:
        return "%si";
    case REG_RDI:
        return "%di";
    case REG_R8:
        return "%r8w";
    case REG_R9:
        return "%r9w";
    case REG_R10:
        return "%r10w";
    case REG_R11:
        return "%r11w";
    case REG_R12:
        return "%r12w";
    case REG_R13:
        return "%r13w";
    case REG_R14:
        return "%r14w";
    case REG_R15:
        return "%r15w";
    default:
        return "INVALID-16";
    }
}

static char *tacc_target_register_as_8(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%al";
    case REG_RBX:
        return "%bl";
    case REG_RCX:
        return "%cl";
    case REG_RDX:
        return "%dl";
    case REG_RSI:
        return "%sil";
    case REG_RDI:
        return "%dil";
    case REG_R8:
        return "%r8b";
    case REG_R9:
        return "%r9b";
    case REG_R10:
        return "%r10b";
    case REG_R11:
        return "%r11b";
    case REG_R12:
        return "%r12b";
    case REG_R13:
        return "%r13b";
    case REG_R14:
        return "%r14b";
    case REG_R15:
        return "%r15b";
    default:
        return "INVALID-16";
    }
}

static char *tacc_target_register_name(enum tacc_target_register reg,
                                       size_t width) {
    switch (width) {
    case 64:
        return tacc_target_register_as_64(reg);
    case 32:
        return tacc_target_register_as_32(reg);
    case 16:
        return tacc_target_register_as_16(reg);
    case 8:
        return tacc_target_register_as_8(reg);
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid width %d", width);
        return NULL;
    }
}

static char *tacc_target_op_suffix(size_t width) {
    switch (width) {
    case 64:
        return "q";
    case 32:
        return "l";
    case 16:
        return "w";
    case 8:
        return "b";
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid width %d", width);
        return NULL;
    }
}

void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    size_t width;
    char *reg;
    char *op_suff;

    width = tacc_type_bit_width(val->type);

    register_place = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
    reg = tacc_target_register_name(register_place, width);
    op_suff = tacc_target_op_suffix(width);
    if (val->value.int_value->high == 0 || width <= 32) {
        tacc_cg_output(state,
                       "\n\t mov%s $0x%x, %s",
                       op_suff,
                       val->value.int_value->low,
                       reg);
    } else {
        tacc_cg_output(
            state, "\n\t movq $0x%x, %s", val->value.int_value->high, reg);
        tacc_cg_output(state, "\n\t shlq $32, %s", reg);
        tacc_cg_output(
            state, "\n\t addq $0x%x, %s", val->value.int_value->low, reg);
    }

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_cg_push_reg(state, reg_place, val->type);
}

void tacc_target_cg_jump_to_return(struct tacc_cg_state *state) {
    tacc_cg_output(state, "\n\t jmp .L%u_epilog", state->func_name);
}

void tacc_target_cg_adjust_top_for_return(struct tacc_cg_state *state,
                                          struct tacc_callitf *itf) {
    int offset;
    if (itf->retval_kind == CALLITF_RETVAL_FLOAT_X87) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state, "\n\t fldt %d(%%rbp)", offset);
        return;
    }
    tacc_assert(ASSERT_ICE, 0, "unknown retval reg class");
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

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            struct tacc_type *ty,
                            tacc_bool is_sext) {
    enum tacc_target_register top_reg;
    struct tacc_slot *slot;
    size_t from_width;
    size_t to_width;
    char *reg_name;
    char *reg_name_2;
    char *op_base;
    char *op_suff_from;
    char *op_suff_to;

    if (is_sext) {
        op_base = "movs";
    } else {
        op_base = "movz";
    }
    slot = tacc_cg_get_top(state);
    from_width = tacc_type_bit_width(slot->ty);
    to_width = tacc_type_bit_width(ty);

    if (from_width == to_width) {
        /* plain sign-conversion */
        return;
    }
    if (from_width == 32 && to_width == 64 && !is_sext) {
        /*
         * HACK: there is no movzlq, but we can move the low register to itself
         * instead
         */
        top_reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
        reg_name = tacc_target_register_name(top_reg, 32);
        tacc_cg_output(state, "\n\t movl %s, %s", reg_name, reg_name);
        return;
    }

    op_suff_from = tacc_target_op_suffix(from_width);
    op_suff_to = tacc_target_op_suffix(to_width);
    top_reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
    reg_name = tacc_target_register_name(top_reg, from_width);
    reg_name_2 = tacc_target_register_name(top_reg, to_width);
    tacc_cg_output(state,
                   "\n\t %s%s%s %s, %s",
                   op_base,
                   op_suff_from,
                   op_suff_to,
                   reg_name,
                   reg_name_2);
}

void tacc_target_cg_narrow_top(struct tacc_cg_state *state,
                               struct tacc_type *to_type,
                               tacc_bool is_sext) {
    TACC_UNUSED(state);
    TACC_UNUSED(to_type);
    TACC_UNUSED(is_sext);

    /* nothing to do, we leave the upper bits indeterminate */
}

void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from,
                                 uint32_t to) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(from, 64);
    reg_name_2 = tacc_target_register_name(to, 64);

    tacc_cg_output(state, "\n\t movq %s, %s", reg_name, reg_name_2);
}
void tacc_target_cg_move_f_reg_reg(struct tacc_cg_state *state,
                                   uint32_t from,
                                   uint32_t to) {
    struct tacc_type *ty;
    char *reg_name;
    char *reg_name_2;

    ty = tacc_cg_top_type(state);
    reg_name = tacc_target_register_as_xmm(from);
    reg_name_2 = tacc_target_register_as_xmm(to);
    if (ty->kind == TYK_DOUBLE) {
        tacc_cg_output(state, "\n\t movupd %s, %s", reg_name, reg_name_2);
    } else {
        tacc_cg_output(state, "\n\t movups %s, %s", reg_name, reg_name_2);
    }
}

void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(reg_a, 64);
    reg_name_2 = tacc_target_register_name(reg_b, 64);

    tacc_cg_output(state, "\n\t xchgq %s, %s", reg_name, reg_name_2);
}

void tacc_target_cg_copy_param(struct tacc_cg_state *state,
                               struct tacc_callitf_part *in_place,
                               struct tacc_local_var *locvar_place) {
    TACC_UNUSED(state);
    TACC_UNUSED(in_place);
    TACC_UNUSED(locvar_place);
    tacc_assert(ASSERT_ICE, 0, "no special argument places");
}

void tacc_target_cg_finalize(struct tacc_cg_state *state) {
    struct tacc_callitf_part_list_entry *param_entry;
    struct tacc_ident_list_entry *param_ident_entry;
    struct tacc_local_var_map_entry *locvar_place;
    size_t i;

    /* function entry with stack at 16k - 8 */
    tacc_cg_output_prelude(state, "\n\t pushq %%rbp");
    /* stack now aligned to 16 bytes */
    tacc_cg_output_prelude(state, "\n\t movq %%rsp, %%rbp");

    for (i = 0; i < tacc_callitf_part_list_len(state->interface->param_parts);
         i = i + 1) {
        param_entry =
            tacc_callitf_part_list_get(state->interface->param_parts, i);
        param_ident_entry = tacc_ident_list_get(state->param_names, i);
        locvar_place =
            tacc_local_var_map_get(state->locals, param_ident_entry->content);
        tacc_cg_copy_param(state, param_entry->content, locvar_place->content);
    }

    /* non-volatile registers not allocated */

    tacc_cg_output_prelude(state,
                           "\n\t subq $%d, %%rsp",
                           (int) (state->num_local_bytes + 0xF) & ~0xF);

    tacc_cg_output(state, "\n\t movq %%rbp, %%rsp");
    tacc_cg_output(state, "\n\t popq %%rbp");
    tacc_cg_output(state, "\n\t ret");
}

void tacc_target_cg_deref_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    size_t load_width;
    struct tacc_slot *slot;
    uint32_t reg;

    load_width = tacc_type_bit_width(int_type);
    slot = tacc_cg_get_top(state);
    reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);

    switch (load_width) {
    case 8:
    case 16:
    case 32:
    case 64:
        tacc_cg_output(state,
                       "\n\t mov%s (%s), %s",
                       tacc_target_op_suffix(load_width),
                       tacc_target_register_as_64(reg),
                       tacc_target_register_name(reg, load_width));
        slot->ty = int_type;
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid load width %d", load_width);
    }
}
void tacc_target_cg_store_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    size_t load_width;
    uint32_t reg;
    uint32_t addr_reg;

    load_width = tacc_type_bit_width(int_type);
    reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
    addr_reg = tacc_cg_ensure_over_is_single(state, REG_VOLATILE & ~reg);

    switch (load_width) {
    case 8:
    case 16:
    case 32:
    case 64:
        tacc_cg_output(state,
                       "\n\t mov%s %s, (%s)",
                       tacc_target_op_suffix(load_width),
                       tacc_target_register_name(reg, load_width),
                       tacc_target_register_as_64(addr_reg));
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid load width %d", load_width);
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
    tacc_cg_output(state, "\n\t leaq %d(%%rbp), %s", var->offset, reg_name);
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
                   "\n\t movabsq $%s, %s",
                   tacc_dynstring_as_str(symbol_name),
                   reg_name);
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
        tacc_cg_output(state, "\n\t andq $-%d, %%rsp", 1 << align_p2);
    }
#ifdef __M2__
    tacc_cg_output(state, "\n\t subq $%d, %%rsp", space);
#else
    tacc_cg_output(state, "\n\t subq $%" PRIsz ", %%rsp", space);
#endif
}

void tacc_target_cg_load_scratch_part(struct tacc_cg_state *state,
                                      int offset,
                                      uint32_t to_reg,
                                      struct tacc_type *ty) {
    size_t width;

    width = tacc_type_bit_width(ty);
    tacc_cg_output(state,
                   "\n\t mov%s %d(%%rbp), %s",
                   tacc_target_op_suffix(width),
                   offset,
                   tacc_target_register_name(to_reg, width));
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
            state, "\n\t movq %d(%%rbp), %%rax", from_fp_offset + (int) i);
        tacc_cg_output(
            state, "\n\t movq %%rax, %d(%%rsp)", to_sp_offset + (int) i);
    }
}

void tacc_target_cg_call_top(struct tacc_cg_state *state) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    tacc_assert(ASSERT_ICE,
                slot->place_kind == PLACE_SCRATCH,
                "expected callee pointer to be in scratch");
    tacc_cg_output(state, "\n\t movq %d(%%rbp), %%rax", slot->place.offset);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t call %%rax");
}

void tacc_target_cg_normalize_retval(struct tacc_cg_state *state,
                                     struct tacc_callitf *itf,
                                     struct tacc_type *return_ty) {
    int offset;

    switch (itf->retval_kind) {
    case CALLITF_RETVAL_FLOAT_X87:
        offset = tacc_cg_alloc_scratch(state, 16, 4);
        tacc_cg_output(state, "\n\t fstpt %d(%%rbp)", offset);
        tacc_cg_push_scratch(
            state,
            offset,
            tacc_get_basic_type(state->compiler->basic_types, TYK_LONGDOUBLE));
        tacc_cg_convert_top(state, return_ty);
        break;
    default:
        tacc_assert(ASSERT_ICE,
                    0,
                    "this retval should have been normalized by generic code");
        break;
    }
}

void tacc_target_cg_store_reg_to_scratch(struct tacc_cg_state *state,
                                         int offset,
                                         uint32_t reg,
                                         struct tacc_type *ty,
                                         tacc_bool in_prelude) {
    if (in_prelude) {
        tacc_cg_output_prelude(
            state,
            "\n\t mov%s %s, %d(%%rbp)",
            tacc_target_op_suffix(tacc_type_bit_width(ty)),
            tacc_target_register_name(reg, tacc_type_bit_width(ty)),
            offset);
    } else {
        tacc_cg_output(state,
                       "\n\t mov%s %s, %d(%%rbp)",
                       tacc_target_op_suffix(tacc_type_bit_width(ty)),
                       tacc_target_register_name(reg, tacc_type_bit_width(ty)),
                       offset);
    }
}

void tacc_target_cg_store_f_reg_to_scratch(struct tacc_cg_state *state,
                                           int offset,
                                           uint32_t reg,
                                           struct tacc_type *ty,
                                           tacc_bool in_prelude) {
    char *op;

    if (ty->kind == TYK_FLOAT) {
        op = "movss";
    } else {
        op = "movsd";
    }

    if (in_prelude) {
        tacc_cg_output_prelude(state,
                               "\n\t %s %s, %d(%%rbp)",
                               op,
                               tacc_target_register_as_xmm(reg),
                               offset);
    } else {
        tacc_cg_output(state,
                       "\n\t %s %s, %d(%%rbp)",
                       op,
                       tacc_target_register_as_xmm(reg),
                       offset);
    }
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

void tacc_target_cg_convert_float(struct tacc_cg_state *state,
                                  struct tacc_type *to_type) {
    uint32_t reg;
    int offset;
    struct tacc_slot *slot;
    struct tacc_target_place_register *reg_place;

    slot = tacc_cg_get_top(state);
    if (tacc_type_is_compatible(slot->ty, to_type)) {
        return;
    }
    if (slot->ty->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        /* alignment of 16 guaranteed, which is also sufficient for fstl */
        tacc_cg_output(state, "\n\t fldt %d(%%rbp)", offset);
        tacc_cg_pop(state);
        reg = tacc_target_cg_alloc_freg(state, REGV_VOLATILE);
        if (to_type->kind == TYK_FLOAT) {
            tacc_cg_output(state, "\n\t fstps %d(%%rbp)", offset);
            tacc_cg_output(state,
                           "\n\t movss %d(%%rbp), %s",
                           offset,
                           tacc_target_register_as_xmm(reg));
        } else {
            tacc_cg_output(state, "\n\t fstpl %d(%%rbp)", offset);
            tacc_cg_output(state,
                           "\n\t movsd %d(%%rbp), %s",
                           offset,
                           tacc_target_register_as_xmm(reg));
        }
        reg_place = tacc_target_place_register_new();
        reg_place->reg = reg;
        tacc_cg_push_freg(state, reg_place, to_type);
        return;
    }

    reg = tacc_cg_ensure_top_is_single_f(state, REGV_VOLATILE);

    if (to_type->kind == TYK_LONGDOUBLE) {
        tacc_cg_pop(state);
        offset = tacc_cg_alloc_scratch(state, 16, 4);
        if (slot->ty->kind == TYK_FLOAT) {
            tacc_cg_output(state,
                           "\n\t movss %s, %d(%%rbp)",
                           tacc_target_register_as_xmm(reg),
                           offset);
            tacc_cg_output(state, "\n\t flds %d(%%rbp)", offset);
        } else {
            tacc_cg_output(state,
                           "\n\t movsd %s, %d(%%rbp)",
                           tacc_target_register_as_xmm(reg),
                           offset);
            tacc_cg_output(state, "\n\t fldl %d(%%rbp)", offset);
        }
        tacc_cg_output(state, "\n\t fstpt -%d(%%rbp)", offset);
        tacc_cg_push_scratch(state, offset, to_type);
        return;
    }
    if (to_type->kind == TYK_DOUBLE) {
        tacc_cg_output(state,
                       "\n\t cvtss2sd %s, %s",
                       tacc_target_register_as_xmm(reg),
                       tacc_target_register_as_xmm(reg));
    } else {
        tacc_cg_output(state,
                       "\n\t cvtsd2ss %s, %s",
                       tacc_target_register_as_xmm(reg),
                       tacc_target_register_as_xmm(reg));
    }
}

void tacc_target_cg_deref_float(struct tacc_cg_state *state,
                                struct tacc_type *float_type) {
    int offset;
    uint32_t ptr_reg;
    uint32_t reg;
    struct tacc_target_place_register *reg_place;

    ptr_reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE);
    if (float_type->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_alloc_scratch(state,
                                       tacc_type_size(float_type),
                                       tacc_type_alignment_p2(float_type));
        tacc_cg_output(
            state, "\n\t fldt (%s)", tacc_target_register_as_64(ptr_reg));
        tacc_cg_output(state, "\n\t fstpt %d(%%rbp)", offset);
        tacc_cg_pop(state);
        tacc_cg_push_scratch(state, offset, float_type);
        return;
    }
    reg = tacc_target_cg_alloc_freg(state, REGV_VOLATILE);
    if (float_type->kind == TYK_DOUBLE) {
        tacc_cg_output(state,
                       "\n\t movsd (%s), %s",
                       tacc_target_register_as_64(ptr_reg),
                       tacc_target_register_as_xmm(reg));
    } else {
        tacc_cg_output(state,
                       "\n\t movss (%s), %s",
                       tacc_target_register_as_64(ptr_reg),
                       tacc_target_register_as_xmm(reg));
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
    if (float_type->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state, "\n\t fldt %d(%%rbp)", offset);
        tacc_cg_output(
            state, "\n\t fstpt (%s)", tacc_target_register_as_64(ptr_reg));
        tacc_cg_pop(state);
        tacc_cg_pop(state);
        return;
    }
    reg = tacc_cg_ensure_top_is_single_f(state, REGV_VOLATILE);
    if (float_type->kind == TYK_DOUBLE) {
        tacc_cg_output(state,
                       "\n\t movsd %s, (%s)",
                       tacc_target_register_as_xmm(reg),
                       tacc_target_register_as_64(ptr_reg));
    } else {
        tacc_cg_output(state,
                       "\n\t movss %s, (%s)",
                       tacc_target_register_as_xmm(reg),
                       tacc_target_register_as_64(ptr_reg));
    }
    tacc_cg_pop(state);
    tacc_cg_pop(state);
}

void tacc_target_cg_float(struct tacc_cg_state *state, int index) {
    int offset;

    offset = tacc_cg_alloc_scratch(state, 16, 4);
    tacc_cg_output(state, "\n\t fldt .Lfloat_%d", index);
    tacc_cg_output(state, "\n\t fstpt %d(%%rbp)", offset);

    tacc_cg_push_scratch(
        state,
        offset,
        tacc_get_basic_type(state->compiler->basic_types, TYK_LONGDOUBLE));
}

void tacc_target_cg_prepare_arg(struct tacc_cg_state *state,
                                struct tacc_slot *slot,
                                struct tacc_callitf_part *itf_part) {
    TACC_UNUSED(state);
    TACC_UNUSED(slot);
    TACC_UNUSED(itf_part);
    tacc_assert(ASSERT_ICE, 0, "no special argument places");
}

void tacc_target_cg_binop(struct tacc_cg_state *state,
                          enum tacc_expr_kind binop_kind,
                          struct tacc_type *output_type) {

    uint32_t l_reg;
    uint32_t r_reg;
    char *op;
    struct tacc_target_place_register *out_place;
    tacc_bool is_signed;
    tacc_bool is_cmp;
    size_t width;

    out_place = tacc_target_place_register_new();
    width = tacc_type_bit_width(output_type);
    is_signed = tacc_type_is_signed(tacc_cg_top_type(state));

    if (binop_kind == EX_DIV || binop_kind == EX_REM) {
        tacc_cg_ensure_over_is_single(state, REG_RAX);
        r_reg = tacc_cg_ensure_top_is_single(
            state, REG_VOLATILE & ~(REG_RAX | REG_RDX));
        tacc_target_cg_alloc_reg(state, REG_RDX);

        tacc_assert(ASSERT_ICE,
                    width >= 32,
                    "div: integer promotions didn't do what I thought");
        if (is_signed) {
            if (width == 32) {
                tacc_cg_output(state, "\n\t cltd");
                tacc_cg_output(
                    state, "\n\t idivl %s", tacc_target_register_as_32(r_reg));
            } else {
                tacc_cg_output(state, "\n\t cqto");
                tacc_cg_output(
                    state, "\n\t idivq %s", tacc_target_register_as_64(r_reg));
            }
        } else {
            if (width == 32) {
                tacc_cg_output(state, "\n\t xor %%edx, %%edx");
                tacc_cg_output(
                    state, "\n\t divl %s", tacc_target_register_as_32(r_reg));
            } else {
                tacc_cg_output(state, "\n\t xor %%rdx, %%rdx");
                tacc_cg_output(
                    state, "\n\t divq %s", tacc_target_register_as_64(r_reg));
            }
        }
        tacc_cg_pop(state);
        tacc_cg_pop(state);
        if (binop_kind == EX_DIV) {
            out_place->reg = REG_RAX;
        } else {
            out_place->reg = REG_RDX;
        }
        tacc_cg_push_reg(state, out_place, output_type);
        return;
    }

    l_reg = tacc_cg_ensure_over_is_single(state, REG_VOLATILE);
    r_reg = tacc_cg_ensure_top_is_single(state, REG_VOLATILE & ~l_reg);

    tacc_cg_pop(state);
    tacc_cg_pop(state);

    is_cmp = 0;
    switch (binop_kind) {
    case EX_ADD:
        op = "add";
        break;
    case EX_SUB:
        op = "sub";
        break;
    case EX_MUL:
        op = "mul";
        break;
    case EX_BAND:
        op = "and";
        break;
    case EX_BOR:
        op = "or";
        break;
    case EX_BXOR:
        op = "xor";
        break;
    case EX_SHL:
    case EX_SHR:
        if (is_signed) {
            if (binop_kind == EX_SHL) {
                op = "sal";
            } else {
                op = "sar";
            }
        } else {
            if (binop_kind == EX_SHL) {
                op = "shl";
            } else {
                op = "shr";
            }
        }

        out_place->reg = l_reg;
        tacc_cg_push_reg(state, out_place, output_type);
        break;
    case EX_EQ:
        op = "sete";
        is_cmp = 1;
        break;
    case EX_NE:
        op = "setne";
        is_cmp = 1;
        break;
    case EX_LE:
        is_cmp = 1;
        if (is_signed) {
            op = "setle";
        } else {
            op = "setbe";
        }
        break;
    case EX_LT:
        is_cmp = 1;
        if (is_signed) {
            op = "setle";
        } else {
            op = "setbe";
        }
        break;
    case EX_GE:
        is_cmp = 1;
        if (is_signed) {
            op = "setge";
        } else {
            op = "setae";
        }
        break;
    case EX_GT:
        is_cmp = 1;
        if (is_signed) {
            op = "setg";
        } else {
            op = "seta";
        }
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "can't generate code for this binop");
        return;
    }

    if (binop_kind == EX_SHL || binop_kind == EX_SHR) {
        tacc_cg_output(state,
                       "\n\t %s%s %s, %s",
                       op,
                       tacc_target_op_suffix(width),
                       tacc_target_register_as_8(r_reg),
                       tacc_target_register_name(l_reg, width));
    } else if (is_cmp) {
        tacc_cg_output(state,
                       "\n\t cmp%s %s, %s",
                       tacc_target_op_suffix(width),
                       tacc_target_register_name(r_reg, width),
                       tacc_target_register_name(l_reg, width));
        tacc_cg_output(
            state, "\n\t %s %s", op, tacc_target_register_as_8(l_reg));
    } else {
        tacc_cg_output(state,
                       "\n\t %s%s %s, %s",
                       op,
                       tacc_target_op_suffix(width),
                       tacc_target_register_name(r_reg, width),
                       tacc_target_register_name(l_reg, width));
    }
    out_place->reg = l_reg;
    tacc_cg_push_reg(state, out_place, output_type);
}

void tacc_target_cg_float_binop(struct tacc_cg_state *state,
                                enum tacc_expr_kind binop_kind,
                                struct tacc_type *output_type) {

    uint32_t l_reg;
    uint32_t r_reg;
    uint32_t out_reg;
    uint32_t out_reg_2;
    int offset;
    tacc_bool is_ldouble;
    tacc_bool is_cmp;
    char *op;
    char *op_suffix;
    struct tacc_target_place_register *out_place;
    struct tacc_type *ty;
    enum tacc_expr_kind actual_binop_kind;

    out_place = tacc_target_place_register_new();
    ty = tacc_cg_top_type(state);
    actual_binop_kind = binop_kind;

    /* to silence spurious initialization warnings */
    out_reg_2 = 0;
    l_reg = 0;

    if (binop_kind == EX_GE || binop_kind == EX_GT) {
        tacc_cg_swap(state);
        if (binop_kind == EX_GE) {
            actual_binop_kind = EX_LE;
        } else {
            actual_binop_kind = EX_LT;
        }
    }

    if (ty->kind == TYK_LONGDOUBLE) {
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state, "\n\t fldt %d(%%rbp)", offset);
        tacc_cg_pop(state);
        offset = tacc_cg_ensure_top_is_scratch(state);
        tacc_cg_output(state, "\n\t fldt %d(%%rbp)", offset);
        tacc_cg_pop(state);
        is_ldouble = 1;
    } else {
        l_reg = tacc_cg_ensure_over_is_single_f(state, REG_VOLATILE);
        r_reg = tacc_cg_ensure_top_is_single_f(state, REG_VOLATILE & ~l_reg);
        tacc_cg_pop(state);
        tacc_cg_pop(state);
        if (ty->kind == TYK_DOUBLE) {
            op_suffix = "d";
        } else {
            op_suffix = "s";
        }
        is_ldouble = 0;
    }

    is_cmp = 0;
    switch (actual_binop_kind) {
    case EX_ADD:
        op = "add";
        break;
    case EX_SUB:
        op = "sub";
        break;
    case EX_MUL:
        op = "mul";
        break;
    case EX_DIV:
        op = "div";
        break;
    case EX_EQ:
    case EX_NE:
        is_cmp = 1;
        op = "ucom";
        out_reg = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
        out_reg_2 = tacc_target_cg_alloc_reg(state, REG_VOLATILE & ~out_reg);
        break;
    case EX_LE:
    case EX_LT:
        is_cmp = 1;
        op = "com";
        out_reg = tacc_target_cg_alloc_reg(state, REG_VOLATILE);
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "can't generate code for this binop");
        return;
    }

    if (is_cmp) {
        if (is_ldouble) {
            tacc_cg_output(state, "\n\t f%spp", op);
        } else {
            tacc_cg_output(state,
                           "\n\t %sis%s %s, %s",
                           op,
                           op_suffix,
                           tacc_target_register_as_xmm(l_reg),
                           tacc_target_register_as_xmm(r_reg));
        }
        switch (actual_binop_kind) {
        case EX_EQ:
            tacc_cg_output(
                state, "\n\t setnp %s", tacc_target_register_as_8(out_reg));
            tacc_cg_output(
                state, "\n\t sete %s", tacc_target_register_as_8(out_reg_2));
            tacc_cg_output(state,
                           "\n\t and %s, %s",
                           tacc_target_register_as_8(out_reg_2),
                           tacc_target_register_as_8(out_reg));
            break;
        case EX_NE:
            tacc_cg_output(
                state, "\n\t setp %s", tacc_target_register_as_8(out_reg));
            tacc_cg_output(
                state, "\n\t setne %s", tacc_target_register_as_8(out_reg_2));
            tacc_cg_output(state,
                           "\n\t or %s, %s",
                           tacc_target_register_as_8(out_reg_2),
                           tacc_target_register_as_8(out_reg));
            break;
        case EX_LT:
            tacc_cg_output(
                state, "seta %s", tacc_target_register_as_8(out_reg));
            break;
        case EX_LE:
            tacc_cg_output(
                state, "setnb %s", tacc_target_register_as_8(out_reg));
            break;
        default:
            tacc_assert(ASSERT_ICE, 0, "bad comparison binop");
        }
        out_place->reg = out_reg;
        tacc_cg_push_reg(state, out_place, output_type);
        return;
    }
    if (is_ldouble) {
        tacc_cg_output(state, "\n\t f%sp %%st(1)", op);
        /* reuse scratch */
        tacc_cg_output(state, "\n\t fstpt %d(%%rbp)", offset);
        tacc_cg_push_scratch(state, offset, output_type);
        tacc_target_place_register_free(out_place);
    } else {
        tacc_cg_output(state,
                       "\n\t %ss%s %s, %s",
                       op,
                       op_suffix,
                       tacc_target_register_as_xmm(r_reg),
                       tacc_target_register_as_xmm(l_reg));
        out_place->reg = l_reg;
        tacc_cg_push_freg(state, out_place, output_type);
    }
}
