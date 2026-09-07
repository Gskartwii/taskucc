#include "target/call_itf.h"
#include "call_itf.h"
#include "target/riscv64-linux/registers.h"
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
        return REG_A0;
    case 1:
        return REG_A1;
    case 2:
        return REG_A2;
    case 3:
        return REG_A3;
    case 4:
        return REG_A4;
    case 5:
        return REG_A5;
    case 6:
        return REG_A6;
    case 7:
        return REG_A7;
    default:
        tacc_assert(0, "ICE: impossible areg %d", index);
        return 0;
    }
}

uint32_t tacc_callitf_float_areg(uint32_t index) {
    switch (index) {
    case 0:
        return REGF_A0;
    case 1:
        return REGF_A1;
    case 2:
        return REGF_A2;
    case 3:
        return REGF_A3;
    case 4:
        return REGF_A4;
    case 5:
        return REGF_A5;
    case 6:
        return REGF_A6;
    case 7:
        return REGF_A7;
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
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
    case TYK_ENUM:
    case TYK_BOOL:
        part->ty = arg_type;
        if (state->int_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            part->place.extra.reg.reg_class = REGC_INT;
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
        part->ty = arg_type;
        if (state->float_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER;
            if (arg_type->kind == TYK_FLOAT) {
                part->place.extra.reg.reg_class = REGC_FLOAT_S;
            } else {
                part->place.extra.reg.reg_class = REGC_FLOAT_D;
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
        part->ty = arg_type;
        state->int_regs_used =
            (uint32_t) tacc_align_up(state->int_regs_used, 1);
        if (state->int_regs_used < 8) {
            part->place.kind = CALLITF_PLACE_REGISTER_PAIR;
            part->place.extra.pair.reg_class = REGC_INT;
            part->place.extra.pair.reg =
                tacc_callitf_areg(state->int_regs_used);
            part->place.extra.pair.reg_2 =
                tacc_callitf_areg(state->int_regs_used + 1);
            state->int_regs_used = state->int_regs_used + 2;
        } else {
            part->place.kind = CALLITF_PLACE_STACK;
            state->used_stack = (uint32_t) tacc_align_up(state->used_stack, 4);
            part->place.extra.stack_offset = (int) (state->used_stack);
            state->used_stack = state->used_stack + 16;
        }
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
        tacc_assert(0, "TODO: call interface for struct/union on riscv64");
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
    state.used_stack = 16; /* ra and fp */
    state.int_regs_used = 0;
    state.float_regs_used = 0;

    ret->retval_kind = CALLITF_RETVAL_REGISTER;
    ret->retval_reg = REG_A0;
    switch (ty->return_type->kind) {
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_PTR:
    case TYK_ENUM:
        ret->retval_reg_class = REGC_INT;
        break;
    case TYK_FLOAT:
        ret->retval_reg_class = REGC_FLOAT_S;
        ret->retval_reg = REGF_A0;
        break;
    case TYK_DOUBLE:
        ret->retval_reg_class = REGC_FLOAT_D;
        ret->retval_reg = REGF_A0;
        break;
    case TYK_LONGDOUBLE:
        ret->retval_kind = CALLITF_RETVAL_REGISTER_PAIR;
        ret->retval_reg_class = REGC_INT;
        ret->retval_reg_2 = REG_A1;
        break;
    case TYK_VOID:
        ret->retval_kind = CALLITF_RETVAL_NONE;
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        tacc_assert(0, "TODO: returning struct/union on riscv64");
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

    ret->frame_offset = 16;

    return ret;
}
