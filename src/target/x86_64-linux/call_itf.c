#include "target/call_itf.h"
#include "call_itf.h"
#include "target/x86_64-linux/registers.h"
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
        return REG_RDI;
    case 1:
        return REG_RSI;
    case 2:
        return REG_RDX;
    case 3:
        return REG_RCX;
    case 4:
        return REG_R8;
    case 5:
        return REG_R9;
    default:
        tacc_assert(0, "ICE: impossible areg %d", index);
        return 0;
    }
}

uint32_t tacc_callitf_float_areg(uint32_t index) {
    switch (index) {
    case 0:
        return REGV_XMM0;
    case 1:
        return REGV_XMM1;
    case 2:
        return REGV_XMM2;
    case 3:
        return REGV_XMM3;
    case 4:
        return REGV_XMM4;
    case 5:
        return REGV_XMM5;
    case 6:
        return REGV_XMM6;
    case 7:
        return REGV_XMM7;
    default:
        tacc_assert(0, "ICE: impossible areg %d", index);
        return 0;
    }
}

static struct tacc_callitf_part *tacc_target_callitf_part_from_arg(
    struct tacc_type *arg_type, struct tacc_callitf_state *state) {
    struct tacc_callitf_part *part;

    part = tacc_callitf_part_new();
    switch (arg_type->kind) {
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
        part->place.extra.reg.reg_class = REGC_INT_B;
        goto alloc_int;
    case TYK_USHORT:
    case TYK_SSHORT:
        part->place.extra.reg.reg_class = REGC_INT_W;
        goto alloc_int;
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
        part->place.extra.reg.reg_class = REGC_INT_L;
        goto alloc_int;
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        part->place.extra.reg.reg_class = REGC_INT_Q;

    alloc_int:
        if (state->int_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            part->place.extra.reg.reg = tacc_callitf_areg(state->int_regs_used);
            state->int_regs_used = state->int_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack_offset = (int) (state->used_stack);
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
        if (state->float_regs_used < 6) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            if (arg_type->kind == TYK_FLOAT) {
                part->place.extra.reg.reg_class = REGC_FLOAT_SSE_S;
            } else {
                part->place.extra.reg.reg_class = REGC_FLOAT_SSE_D;
            }
            part->place.extra.reg.reg =
                tacc_callitf_float_areg(state->int_regs_used);
            state->float_regs_used = state->float_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack_offset = (int) (state->used_stack);
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_LONGDOUBLE:
        part->place.kind = CALLITF_PLACE_STACK;
        state->used_stack = (uint32_t) tacc_align_up(state->used_stack, 4);
        part->place.extra.stack_offset = (int) (state->used_stack);
        state->used_stack = state->used_stack + 16;
        break;
    case TYK_ARRAY:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
    case TYK_FN:
        tacc_assert(0, "ICE: unnormalized function param type");
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(0, "TODO: call interface for struct/union on x86_64");
        break;
    case TYK_VOID:
        tacc_assert(0, "function cannot take void as parameter");
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
    state.float_regs_used = 0;

    ret->retval_kind = CALLITF_RETVAL_REGISTER;
    ret->retval_reg = REG_RAX;
    switch (ty->return_type->kind) {
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
        ret->retval_reg_class = REGC_INT_B;
        break;
    case TYK_USHORT:
    case TYK_SSHORT:
        ret->retval_reg_class = REGC_INT_W;
        break;
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
        ret->retval_reg_class = REGC_INT_L;
        break;
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        ret->retval_reg_class = REGC_INT_Q;
        break;
    case TYK_FLOAT:
        ret->retval_reg_class = REGC_FLOAT_SSE_S;
        ret->retval_reg = REGV_XMM0;
        break;
    case TYK_DOUBLE:
        ret->retval_reg_class = REGC_FLOAT_SSE_D;
        ret->retval_reg = REGV_XMM0;
        break;
    case TYK_LONGDOUBLE:
        ret->retval_reg_class = REGC_FLOAT_X87;
        ret->retval_reg = 0;
        break;
    case TYK_VOID:
        ret->retval_kind = CALLITF_RETVAL_NONE;
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(0, "TODO: returning struct/union on x86_64");
        break;
    case TYK_FN:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
    case TYK_ARRAY:
        tacc_assert(0, "invalid return type");
        break;
    }

    for (i = 0; i < tacc_type_list_len(ty->param_types); i = i + 1) {
        ty_entry = tacc_type_list_get(ty->param_types, i);
        tacc_callitf_part_list_push(
            ret->param_parts,
            tacc_target_callitf_part_from_arg(ty_entry->content, &state));
    }

    return ret;
}
