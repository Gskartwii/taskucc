#include "target/call_itf.h"
#include "call_itf.h"
#include "target/aarch64-linux/registers.h"
#include "type.h"
#include "util.h"

struct tacc_callitf_state {
    uint32_t int_regs_used;
    uint32_t float_regs_used;
    uint32_t used_stack;
};

uint32_t tacc_callitf_areg(uint32_t index) {
    switch (index) {
    case 0:
        return REG_X0;
    case 1:
        return REG_X1;
    case 2:
        return REG_X2;
    case 3:
        return REG_X3;
    case 4:
        return REG_X4;
    case 5:
        return REG_X5;
    case 6:
        return REG_X6;
    case 7:
        return REG_X7;
    default:
        tacc_assert(ASSERT_ICE, 0, "impossible areg %d", index);
        return 0;
    }
}

uint32_t tacc_callitf_float_areg(uint32_t index) {
    switch (index) {
    case 0:
        return REGF_V0;
    case 1:
        return REGF_V1;
    case 2:
        return REGF_V2;
    case 3:
        return REGF_V3;
    case 4:
        return REGF_V4;
    case 5:
        return REGF_V5;
    case 6:
        return REGF_V6;
    case 7:
        return REGF_V7;
    default:
        tacc_assert(ASSERT_ICE, 0, "impossible areg %d", index);
        return 0;
    }
}

static struct tacc_callitf_part *tacc_target_callitf_part_from_arg(
    struct tacc_type *arg_type, struct tacc_callitf_state *state) {
    struct tacc_callitf_part *part;

    part = tacc_callitf_part_new();
    switch (arg_type->kind) {
    case TYK_BOOL:
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
    case TYK_ULONG:
    case TYK_SLONG:
        part->ty = arg_type;
        if (state->int_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            part->place.extra.reg.reg_class = REGC_INT_W;
            part->place.extra.reg.reg = tacc_callitf_areg(state->int_regs_used);
            state->int_regs_used = state->int_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack.offset = (int) (state->used_stack);
            part->place.extra.stack.align_p2 = 3;
            part->place.extra.stack.size = 8;
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        part->ty = arg_type;
        if (state->int_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            part->place.extra.reg.reg_class = REGC_INT_X;
            part->place.extra.reg.reg = tacc_callitf_areg(state->int_regs_used);
            state->int_regs_used = state->int_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack.offset = (int) (state->used_stack);
            part->place.extra.stack.align_p2 = 3;
            part->place.extra.stack.size = 8;
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
        part->ty = arg_type;
        if (state->float_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            if (arg_type->kind == TYK_FLOAT) {
                part->place.extra.reg.reg_class = REGC_FLOAT_S;
            } else if (arg_type->kind == TYK_DOUBLE) {
                part->place.extra.reg.reg_class = REGC_FLOAT_D;
            } else {
                part->place.extra.reg.reg_class = REGC_FLOAT_Q;
            }
            part->place.extra.reg.reg =
                tacc_callitf_float_areg(state->int_regs_used);
            state->float_regs_used = state->float_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            if (arg_type->kind == TYK_LONGDOUBLE) {
                state->used_stack =
                    (uint32_t) tacc_align_up(state->used_stack, 4);
                part->place.extra.stack.offset = (int) (state->used_stack);
                part->place.extra.stack.align_p2 = 4;
                part->place.extra.stack.size = 16;
                state->used_stack = state->used_stack + 16;
            } else {
                part->place.extra.stack.offset = (int) (state->used_stack);
                part->place.extra.stack.align_p2 = 3;
                part->place.extra.stack.size = 8;
                state->used_stack = state->used_stack + 8;
            }
        }
        break;
    case TYK_ARRAY:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
    case TYK_FN:
        tacc_assert(ASSERT_ICE, 0, "unnormalized function param type");
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(
            ASSERT_TODO, 0, "call interface for struct/union on aarch64");
        break;
    case TYK_VOID:
        tacc_assert(ASSERT_DIAG, 0, "function cannot take void as parameter");
        break;
    }

    return part;
}

struct tacc_callitf *
tacc_target_callitf_from_func_type(struct tacc_function_type *ty) {
    struct tacc_callitf *ret;
    struct tacc_callitf_state state;
    struct tacc_type_list_entry *ty_entry;
    size_t i;

    ret = tacc_callitf_new();
    state.used_stack = 0;
    state.int_regs_used = 0;

    ret->retval_kind = CALLITF_RETVAL_REGISTER;
    ret->retval_reg = REG_X0;
    switch (ty->return_type->kind) {
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
        ret->retval_reg_class = REGC_INT_W;
        break;
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        ret->retval_reg_class = REGC_INT_X;
        break;
    case TYK_FLOAT:
        ret->retval_reg_class = REGC_FLOAT_S;
        ret->retval_reg = REGF_V0;
        break;
    case TYK_DOUBLE:
        ret->retval_reg_class = REGC_FLOAT_D;
        ret->retval_reg = REGF_V0;
        break;
    case TYK_LONGDOUBLE:
        ret->retval_reg_class = REGC_FLOAT_Q;
        ret->retval_reg = REGF_V0;
        break;
    case TYK_VOID:
        ret->retval_kind = CALLITF_RETVAL_NONE;
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(ASSERT_TODO, 0, "returning struct/union on riscv64");
        break;
    case TYK_FN:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
    case TYK_ARRAY:
        tacc_assert(ASSERT_DIAG, 0, "invalid return type");
        break;
    }

    for (i = 0; i < tacc_type_list_len(ty->param_types); i = i + 1) {
        ty_entry = tacc_type_list_get(ty->param_types, i);
        tacc_callitf_part_list_push(
            ret->param_parts,
            tacc_target_callitf_part_from_arg(ty_entry->content, &state));
    }

    ret->frame_offset = 0;

    return ret;
}
