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
        tacc_assert(ASSERT_ICE, 0, "impossible areg %d", index);
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
        tacc_assert(ASSERT_ICE, 0, "impossible areg %d", index);
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
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        part->access_as_type = arg_type;
        if (state->int_regs_used < 6) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            part->place.extra.reg.reg = tacc_callitf_areg(state->int_regs_used);
            state->int_regs_used = state->int_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack.offset = (int) (state->used_stack);
            part->place.extra.stack.size = 8;
            part->place.extra.stack.align_p2 = 3;
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
        part->access_as_type = arg_type;
        if (state->float_regs_used < 6) {
            part->place.kind = CALLITF_PLACE_FLOAT_REGISTER;
            part->place.extra.reg.reg =
                tacc_callitf_float_areg(state->int_regs_used);
            state->float_regs_used = state->float_regs_used + 1;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            part->place.extra.stack.offset = (int) (state->used_stack);
            part->place.extra.stack.size = 8;
            part->place.extra.stack.align_p2 = 3;
            state->used_stack = state->used_stack + 8;
        }
        break;
    case TYK_LONGDOUBLE:
        part->access_as_type = arg_type;
        part->place.kind = CALLITF_PLACE_STACK;
        state->used_stack = (uint32_t) tacc_align_up(state->used_stack, 4);
        part->place.extra.stack.offset = (int) (state->used_stack);
        part->place.extra.stack.size = 16;
        part->place.extra.stack.align_p2 = 4;
        state->used_stack = state->used_stack + 16;
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
            ASSERT_TODO, 0, "call interface for struct/union on x86_64");
        break;
    case TYK_VOID:
        tacc_assert(ASSERT_DIAG, 0, "function cannot take void as parameter");
        break;
    }

    return part;
}

struct tacc_callitf *tacc_target_callitf_from_func_type(
    struct tacc_type_list *basic_types, struct tacc_function_type *ty) {
    struct tacc_callitf *ret;
    struct tacc_callitf_state state;
    struct tacc_type_list_entry *ty_entry;
    struct tacc_callitf_part *part;
    size_t i;

    TACC_UNUSED(basic_types);

    ret = tacc_callitf_new();
    /* +8 for old rbp, +8 for return address */
    state.used_stack = 16;
    state.int_regs_used = 0;
    state.float_regs_used = 0;
    ret->implicit_stack_use = 16;

    switch (ty->return_type->kind) {
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ENUM:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
        ret->retval_kind = CALLITF_RETVAL_REGISTER;
        ret->retval_reg = REG_RAX;
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
        ret->retval_kind = CALLITF_RETVAL_FLOAT_REGISTER;
        ret->retval_reg = REGV_XMM0;
        break;
    case TYK_LONGDOUBLE:
        ret->retval_kind = CALLITF_RETVAL_FLOAT_X87;
        ret->retval_reg = 0;
        break;
    case TYK_VOID:
        ret->retval_kind = CALLITF_RETVAL_NONE;
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(ASSERT_TODO, 0, "returning struct/union on x86_64");
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
        part = tacc_target_callitf_part_from_arg(ty_entry->content, &state);
        part->param_idx = i;
        tacc_callitf_part_list_push(ret->param_parts, part);
    }

    ret->frame_offset = 0;

    return ret;
}
