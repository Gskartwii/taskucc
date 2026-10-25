#include "initializer.h"
#include "decl.h"
#include "dynarray.h"
#include "dynstring.h"
#include "expr.h"
#include "machine.h"
#include "soft_u64.h"
#include "type.h"
#include "util.h"

MK_DYNARRAY_OVER(tacc_init_command_list,
                 tacc_init_command_list_entry,
                 struct tacc_init_command *,
                 tacc_init_command_list_new,
                 tacc_init_command_list_init,
                 tacc_init_command_list_get,
                 tacc_init_command_list_push,
                 tacc_init_command_list_pop,
                 tacc_init_command_list_len,
                 tacc_init_command_free,
                 tacc_init_command_list_free)

struct tacc_init_eval_ctx {
    struct tacc_eval_context *const_eval_ctx;
    struct tacc_init_command_list *init_commands;
    struct tacc_init_position_list *position;
};

static struct tacc_init_command *tacc_init_command_new(void) {
    struct tacc_init_command *cmd;

    cmd = tacc_malloc(sizeof(struct tacc_init_command));
    cmd->offset = 0;
    cmd->stored_expr = NULL;
    cmd->as_type = NULL;

    return cmd;
}

static void tacc_init_store_val(struct tacc_expr *expr,
                                struct tacc_type *as_type,
                                int offset,
                                struct tacc_init_eval_ctx *ctx) {
    struct tacc_init_command *cmd;

    cmd = tacc_init_command_new();
    cmd->offset = offset;
    cmd->stored_expr = expr;
    cmd->as_type = as_type;

    tacc_init_command_list_push(ctx->init_commands, cmd);
}

struct tacc_init_position {
    struct tacc_type *type;
    int start_offset;
    size_t position_in_parent;
};

void tacc_init_position_free(struct tacc_init_position *position) {
    tacc_free(position);
}

DECL_DYNARRAY_OVER(tacc_init_position_list,
                   tacc_init_position_list_entry,
                   struct tacc_init_position *,
                   tacc_init_position_list_new,
                   tacc_init_position_list_init,
                   tacc_init_position_list_get,
                   tacc_init_position_list_push,
                   tacc_init_position_list_pop,
                   tacc_init_position_list_len,
                   tacc_init_position_list_free)
MK_DYNARRAY_OVER(tacc_init_position_list,
                 tacc_init_position_list_entry,
                 struct tacc_init_position *,
                 tacc_init_position_list_new,
                 tacc_init_position_list_init,
                 tacc_init_position_list_get,
                 tacc_init_position_list_push,
                 tacc_init_position_list_pop,
                 tacc_init_position_list_len,
                 tacc_init_position_free,
                 tacc_init_position_list_free)

static struct tacc_init_position *tacc_init_position_new(void) {
    struct tacc_init_position *position;

    position = tacc_malloc(sizeof(struct tacc_init_position));
    position->start_offset = 0;
    position->position_in_parent = 0;
    position->type = NULL;

    return position;
}

static void tacc_init_position_rewind(struct tacc_init_position_list *position,
                                      size_t min_depth) {
    while (tacc_init_position_list_len(position) > min_depth) {
        tacc_init_position_free(tacc_init_position_list_pop(position));
    }
}

static void tacc_init_dig_to_scalar(struct tacc_init_eval_ctx *ctx) {
    struct tacc_init_position_list_entry *entry;
    struct tacc_field_list_entry *field_entry;
    struct tacc_init_position *position;
    enum tacc_type_kind kind;

    while (1) {
        entry = tacc_init_position_list_get(
            ctx->position, tacc_init_position_list_len(ctx->position) - 1);
        if (tacc_type_is_scalar(entry->content->type)) {
            break;
        }
        kind = entry->content->type->kind;
        position = tacc_init_position_new();
        position->start_offset = entry->content->start_offset;
        position->position_in_parent = 0;

        if (kind == TYK_ARRAY || kind == TYK_INCOMPLETE_ARRAY) {
            position->type = entry->content->type->extra.array->element_type;
        } else {
            tacc_assert(
                ASSERT_ICE,
                kind == TYK_UNION || kind == TYK_STRUCT,
                "this should be impossible: not a scalar, array, union or struct");
            if (kind == TYK_UNION) {
                field_entry = tacc_field_list_get(
                    entry->content->type->extra.onion->fields, 0);
            } else {
                field_entry = tacc_field_list_get(
                    entry->content->type->extra.structure->fields, 0);
            }
            position->type = field_entry->content->type;
        }
        tacc_init_position_list_push(ctx->position, position);
    }
}

static void tacc_init_position_update(struct tacc_designator_list *chain,
                                      struct tacc_init_eval_ctx *ctx,
                                      size_t min_depth) {
    struct tacc_init_position_list_entry *entry;
    struct tacc_init_position *position;
    struct tacc_designator_list_entry *designator_entry;
    struct tacc_type *ty;
    struct tacc_val *val;
    struct tacc_field *field;
    size_t i;

    tacc_init_position_rewind(ctx->position, min_depth);
    entry = tacc_init_position_list_get(ctx->position, min_depth - 1);
    position = entry->content;
    ty = entry->content->type;
    for (i = 0; i < tacc_designator_list_len(chain); i = i + 1) {
        designator_entry = tacc_designator_list_get(chain, i);
        if (designator_entry->content->designator_kind == DESIGNATOR_EXPR) {
            val =
                tacc_expr_const_eval(designator_entry->content->designator.expr,
                                     ctx->const_eval_ctx);
            tacc_assert(ASSERT_DIAG,
                        tacc_val_is_integral(val),
                        "designator expression must be an integer constant");
            tacc_assert(ASSERT_DIAG,
                        ty->kind == TYK_ARRAY ||
                            ty->kind == TYK_INCOMPLETE_ARRAY,
                        "index designator used on non-array type");
            if (ty->kind == TYK_ARRAY) {
                tacc_assert(ASSERT_DIAG,
                            tacc_u64_ult(val->value.int_value,
                                         ty->extra.array->dimension),
                            "designator points out of bounds of array");
            }
            ty = ty->extra.array->element_type;
            tacc_assert(
                ASSERT_TODO, val->value.int_value->high == 0, "too large type");

            position = tacc_init_position_new();
            position->start_offset =
                position->start_offset + (int) (val->value.int_value->low);
            position->type = ty;
            tacc_init_position_list_push(ctx->position, position);
            continue;
        }
        tacc_assert(ASSERT_DIAG,
                    ty->kind == TYK_UNION || ty->kind == TYK_STRUCT,
                    "named designator used on non-struct/union type");
        if (ty->kind == TYK_UNION) {
            field = tacc_field_list_find(
                ty->extra.onion->fields,
                tacc_dynstring_as_str(
                    designator_entry->content->designator.name));
        } else {
            field = tacc_field_list_find(
                ty->extra.structure->fields,
                tacc_dynstring_as_str(
                    designator_entry->content->designator.name));
        }
        tacc_assert(
            ASSERT_DIAG, field != NULL, "designated field not found type");
        tacc_assert(
            ASSERT_TODO, !field->is_bitfield, "bitfield in initializer");
        ty = field->type;
        position = tacc_init_position_new();
        position->start_offset = position->start_offset + (int) (field->offset);
        position->type = ty;
        tacc_init_position_list_push(ctx->position, position);
    }
}

static struct tacc_type *
tacc_init_position_scalar_ty(struct tacc_init_eval_ctx *ctx) {
    struct tacc_init_position_list_entry *entry;

    tacc_init_dig_to_scalar(ctx);
    entry = tacc_init_position_list_get(
        ctx->position, tacc_init_position_list_len(ctx->position) - 1);
    tacc_assert(ASSERT_ICE,
                tacc_type_is_scalar(entry->content->type),
                "init_position_scalar_ty: not a scalar type");

    return entry->content->type;
}

static int tacc_init_position_offset(struct tacc_init_position_list *pos_list) {
    struct tacc_init_position_list_entry *entry;

    entry = tacc_init_position_list_get(
        pos_list, tacc_init_position_list_len(pos_list) - 1);

    return entry->content->start_offset;
}

static tacc_bool tacc_init_position_advance(struct tacc_init_eval_ctx *ctx,
                                            size_t min_depth) {
    struct tacc_init_position_list_entry *entry;
    struct tacc_field_list_entry *field_entry;
    struct tacc_init_position *position;
    struct tacc_type *ty;
    size_t last_position_in_parent;
    int last_offset;
    size_t depth;

    while (1) {
        position = tacc_init_position_list_pop(ctx->position);
        last_position_in_parent = position->position_in_parent;
        last_offset = position->start_offset;
        tacc_init_position_free(position);
        position = NULL;

        depth = tacc_init_position_list_len(ctx->position);
        if (depth < min_depth) {
            /* end of object */
            return 1;
        }
        entry = tacc_init_position_list_get(ctx->position, depth - 1);
        ty = entry->content->type;
        if (ty->kind == TYK_ARRAY || ty->kind == TYK_INCOMPLETE_ARRAY) {
            if (ty->kind == TYK_ARRAY) {
                if (last_position_in_parent >=
                    (ty->extra.array->dimension->low - 1)) {
                    /* rewind */
                    continue;
                }
            }
            position = tacc_init_position_new();
            position->position_in_parent = last_position_in_parent + 1;
            position->start_offset =
                last_offset +
                (int) (tacc_type_size(ty->extra.array->element_type));
            position->type = ty->extra.array->element_type;
            tacc_init_position_list_push(ctx->position, position);
            break;
        }
        if (ty->kind == TYK_UNION) {
            continue;
        }
        tacc_assert(ASSERT_ICE,
                    ty->kind == TYK_STRUCT,
                    "advancing init position within invalid type");
        if (last_position_in_parent >=
            (tacc_field_list_len(ty->extra.structure->fields) - 1)) {
            continue;
        }
        field_entry = tacc_field_list_get(ty->extra.structure->fields,
                                          last_position_in_parent + 1);
        position = tacc_init_position_new();
        position->position_in_parent = last_position_in_parent + 1;
        position->start_offset =
            entry->content->start_offset + (int) (field_entry->content->offset);
        position->type = field_entry->content->type;
        tacc_init_position_list_push(ctx->position, position);
        break;
    }
    return 0;
}

static void tacc_init_gen_commands(struct tacc_initializer *initializer,
                                   struct tacc_init_eval_ctx *ctx) {
    struct tacc_sub_initializer_list_entry *entry;
    struct tacc_type *next_ty;
    size_t i;
    size_t entry_depth;
    tacc_bool end_of_object;

    entry_depth = tacc_init_position_list_len(ctx->position);

    end_of_object = 0;
    for (i = 0; i < tacc_sub_initializer_list_len(initializer->value.list);
         i = i + 1) {
        entry = tacc_sub_initializer_list_get(initializer->value.list, i);
        if (entry->content->designator_chain != NULL) {
            tacc_init_position_update(
                entry->content->designator_chain, ctx, entry_depth);
        } else {
            tacc_assert(ASSERT_DIAG,
                        !end_of_object,
                        "initializer continues past end of object");
        }
        if (entry->content->value->plain_expr) {
            next_ty = tacc_init_position_scalar_ty(ctx);
            tacc_init_store_val(entry->content->value->value.expr,
                                next_ty,
                                tacc_init_position_offset(ctx->position),
                                ctx);
        } else {
            tacc_init_gen_commands(entry->content->value, ctx);
        }
        end_of_object = tacc_init_position_advance(ctx, entry_depth);
    }
}

static void tacc_init_make_array_complete(struct tacc_type *initialized_type,
                                          struct tacc_init_eval_ctx *ctx) {
    struct tacc_init_command_list_entry *entry;
    size_t max_size;
    size_t end_size;
    size_t type_size;
    size_t i;

    max_size = 0;

    for (i = 0; i < tacc_init_command_list_len(ctx->init_commands); i = i + 1) {
        entry = tacc_init_command_list_get(ctx->init_commands, i);
        end_size = ((size_t) (entry->content->offset)) +
                   tacc_type_size(entry->content->as_type);
        if (end_size > max_size) {
            max_size = end_size;
        }
    }

    /* round up to complete units */
    type_size = tacc_type_size(initialized_type->extra.array->element_type);
    max_size = max_size + (type_size - (max_size % type_size));

    /* make into complete array type */
    initialized_type->kind = TYK_ARRAY;
    initialized_type->extra.array->dimension = tacc_u64_new();
    tacc_u64_from_u32(initialized_type->extra.array->dimension,
                      (uint32_t) max_size);
}

void tacc_init_command_free(struct tacc_init_command *cmd) { tacc_free(cmd); }

struct tacc_init_command_list *
tacc_init_evaluate_initializer(struct tacc_initializer *initializer,
                               struct tacc_type *initialized_type,
                               struct tacc_eval_context *eval_ctx) {
    struct tacc_init_eval_ctx ctx;
    struct tacc_init_position *base_position;

    ctx.const_eval_ctx = eval_ctx;
    ctx.init_commands = tacc_init_command_list_new();
    ctx.position = tacc_init_position_list_new();

    base_position = tacc_init_position_new();
    base_position->position_in_parent = 0;
    base_position->start_offset = 0;
    base_position->type = initialized_type;

    tacc_init_position_list_push(ctx.position, base_position);

    if (initializer->plain_expr) {
        tacc_assert(ASSERT_DIAG,
                    tacc_type_is_scalar(initialized_type),
                    "unbracketed initializer for non-scalar type");
        tacc_init_store_val(initializer->value.expr, initialized_type, 0, &ctx);
    } else {
        tacc_init_gen_commands(initializer, &ctx);
    }
    tacc_init_position_list_free(ctx.position);
    tacc_free(ctx.position);

    if (initialized_type->kind == TYK_INCOMPLETE_ARRAY) {
        tacc_init_make_array_complete(initialized_type, &ctx);
    }

    return ctx.init_commands;
}
