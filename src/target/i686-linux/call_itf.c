#include "target/call_itf.h"
#include "call_itf.h"
#include "target/i686-linux/registers.h"
#include "type.h"

struct tacc_callitf_state {
    uint32_t used_stack;
};

static struct tacc_callitf_part *tacc_target_callitf_part_from_arg(
    struct tacc_type *arg_type, struct tacc_callitf_state *state) {
    struct tacc_callitf_part *part;

    part = tacc_callitf_part_new();

    part->ty = arg_type;
    part->place.kind = CALLITF_PLACE_STACK;
    part->place.extra.stack_offset = (int) (state->used_stack);
    state->used_stack = state->used_stack +
                        (uint32_t) tacc_align_up(tacc_type_size(arg_type), 2);

    return part;
}

struct tacc_callitf *
tacc_target_callitf_from_func_type(struct tacc_function_type *ty) {
    struct tacc_callitf *ret;
    struct tacc_callitf_state state;
    struct tacc_type_list_entry *ty_entry;
    size_t i;

    ret = tacc_callitf_new();
    /* args start at ebp + 8 */
    state.used_stack = 8;

    ret->retval_kind = CALLITF_RETVAL_REGISTER;
    ret->retval_reg = REG_EAX;
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
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_PTR:
        /* TODO: for enums, consider if representation type differs from int */
    case TYK_ENUM:
        ret->retval_reg_class = REGC_INT_D;
        break;
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
        ret->retval_kind = CALLITF_RETVAL_REGISTER_PAIR;
        ret->retval_reg_class = REGC_INT_D;
        ret->retval_reg_2 = REG_EDX;
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
        ret->retval_reg_class = REGC_FLOAT_X87;
        ret->retval_reg = 0;
        break;
    case TYK_VOID:
        ret->retval_kind = CALLITF_RETVAL_NONE;
        break;
    case TYK_STRUCT:
    case TYK_UNION:
        ret->retval_kind = CALLITF_RETVAL_OUTPARAM;
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
