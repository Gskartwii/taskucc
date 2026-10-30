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

    part->access_as_type = arg_type;
    part->place.kind = CALLITF_PLACE_STACK;
    part->place.extra.stack.offset = (int) (state->used_stack);
    part->place.extra.stack.align_p2 = 2;
    part->place.extra.stack.size = tacc_type_size(arg_type);
    if (part->place.extra.stack.size < 4) {
        part->place.extra.stack.size = 4;
    }
    state->used_stack = state->used_stack +
                        (uint32_t) tacc_align_up(tacc_type_size(arg_type), 2);

    return part;
}

static struct tacc_callitf *tacc_target_callitf_begin(
    struct tacc_callitf_state *state, struct tacc_type *return_type) {
    struct tacc_callitf *ret;

    ret = tacc_callitf_new();
    ret->frame_grows_upward = 0;
    /* args start at ebp + 8 */
    state->used_stack = 8;
    ret->implicit_stack_use = 8;
    ret->frame_offset = 12;

    switch (return_type->kind) {
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_BOOL:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_PTR:
        /* TODO: for enums, consider if representation type differs from int */
    case TYK_ENUM:
        ret->retval_kind = CALLITF_RETVAL_REGISTER;
        ret->retval_reg = REG_EAX;
        break;
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
        ret->retval_kind = CALLITF_RETVAL_REGISTER_PAIR;
        ret->retval_reg = REG_EAX;
        ret->retval_reg_2 = REG_EDX;
        break;
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
        ret->retval_kind = CALLITF_RETVAL_FLOAT_X87;
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
        tacc_assert(ASSERT_DIAG, 0, "invalid return type");
        break;
    }

    return ret;
}

struct tacc_callitf *tacc_target_callitf_from_params(
    struct tacc_type_list *basic_types, struct tacc_function_type *ty) {
    struct tacc_callitf *ret;
    struct tacc_callitf_state state;
    struct tacc_type_list_entry *ty_entry;
    struct tacc_callitf_part *part;
    size_t i;

    TACC_UNUSED(basic_types);

    tacc_assert(ASSERT_TODO, !ty->is_vararg, "TODO: vararg in callitf");
    ret = tacc_target_callitf_begin(&state, ty->return_type);

    for (i = 0; i < tacc_type_list_len(ty->param_types); i = i + 1) {
        ty_entry = tacc_type_list_get(ty->param_types, i);
        part = tacc_target_callitf_part_from_arg(ty_entry->content, &state);
        part->param_idx = i;
        tacc_callitf_part_list_push(ret->param_parts, part);
    }

    return ret;
}

void tacc_target_callitf_free(struct tacc_target_callitf *itf) {
    TACC_UNUSED(itf);

    tacc_assert(ASSERT_ICE, 0, "no i686 target-specific callitf");
}

struct tacc_callitf *
tacc_target_callitf_at_call_site(struct tacc_type_list *basic_types,
                                 struct tacc_function_type *ty,
                                 struct tacc_type_list *vararg_types) {
    struct tacc_callitf *ret;
    struct tacc_callitf_state state;
    struct tacc_type_list_entry *ty_entry;
    struct tacc_callitf_part *part;
    size_t i;
    size_t num_named_params;

    TACC_UNUSED(basic_types);
    ret = tacc_target_callitf_begin(&state, ty->return_type);

    if (ty->param_types != NULL) {
        num_named_params = tacc_type_list_len(ty->param_types);
        for (i = 0; i < num_named_params; i = i + 1) {
            ty_entry = tacc_type_list_get(ty->param_types, i);
            part = tacc_target_callitf_part_from_arg(ty_entry->content, &state);
            part->param_idx = i;
            tacc_callitf_part_list_push(ret->param_parts, part);
        }
    } else {
        num_named_params = 0;
    }
    if (vararg_types != NULL) {
        for (i = 0; i < tacc_type_list_len(vararg_types); i = i + 1) {
            ty_entry = tacc_type_list_get(vararg_types, i);
            part = tacc_target_callitf_part_from_arg(ty_entry->content, &state);
            part->param_idx = num_named_params + i;
            tacc_callitf_part_list_push(ret->param_parts, part);
        }
    }

    return ret;
}
