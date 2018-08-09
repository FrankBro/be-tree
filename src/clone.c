#include <string.h>

#include "ast.h"
#include "clone.h"
#include "utils.h"

static struct attr_var clone_attr_var(struct attr_var orig)
{
    struct attr_var clone = { .attr = strdup(orig.attr), .var = orig.var };
    return clone;
}

static struct compare_value clone_compare_value(struct compare_value orig)
{
    struct compare_value clone = { .value_type = orig.value_type };
    switch(orig.value_type) {
        case AST_COMPARE_VALUE_INTEGER: 
            clone.integer_value = orig.integer_value;
            break;
        case AST_COMPARE_VALUE_FLOAT:
            clone.float_value = orig.float_value;
            break;
        default:
            switch_default_error("Invalid compare value type");
            break;
    }
    return clone;
}

static struct ast_node* clone_compare(betree_pred_t id, struct ast_compare_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_COMPARE_EXPR;
    clone->compare_expr.attr_var = clone_attr_var(orig.attr_var);
    clone->compare_expr.op = orig.op;
    clone->compare_expr.value = clone_compare_value(orig.value);
    return clone;
}

static struct string_value clone_string_value(struct string_value orig)
{
    struct string_value clone = { .string = strdup(orig.string), .var = orig.var, .str = orig.str };
    return clone;
}

static struct equality_value clone_equality_value(struct equality_value orig)
{
    struct equality_value clone = { .value_type = orig.value_type };
    switch(orig.value_type) {
        case AST_EQUALITY_VALUE_INTEGER:
            clone.integer_value = orig.integer_value;
            break;
        case AST_EQUALITY_VALUE_FLOAT:
            clone.float_value = orig.float_value;
            break;
        case AST_EQUALITY_VALUE_STRING:
            clone.string_value = clone_string_value(orig.string_value);
            break;
        default:
            switch_default_error("Invalid equality value type");
            break;
    }
    return clone;
}

static struct ast_node* clone_equality(betree_pred_t id, struct ast_equality_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_EQUALITY_EXPR;
    clone->equality_expr.attr_var = clone_attr_var(orig.attr_var);
    clone->equality_expr.op = orig.op;
    clone->equality_expr.value = clone_equality_value(orig.value);
    return clone;
}

struct ast_node* clone_node(const struct ast_node* node);

static struct ast_node* clone_bool(betree_pred_t id, struct ast_bool_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_BOOL_EXPR;
    clone->bool_expr.op = orig.op;
    switch(orig.op) {
        case AST_BOOL_OR:
        case AST_BOOL_AND: {
            struct ast_node* clone_lhs = clone_node(orig.binary.lhs);
            struct ast_node* clone_rhs = clone_node(orig.binary.rhs);
            clone->bool_expr.binary.lhs = clone_lhs;
            clone->bool_expr.binary.rhs = clone_rhs;
            break;
        }
        case AST_BOOL_NOT: {
            struct ast_node* clone_expr = clone_node(orig.unary.expr);
            clone->bool_expr.unary.expr = clone_expr;
            break;
        }
        case AST_BOOL_VARIABLE:
            clone->bool_expr.variable = clone_attr_var(orig.variable);
            break;
        default:
            switch_default_error("Invalid bool op");
            break;
    }
    return clone;
}

static struct set_left_value clone_set_left_value(struct set_left_value orig)
{
    struct set_left_value clone = { .value_type = orig.value_type };
    switch(orig.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            clone.integer_value = orig.integer_value;
            break;
        case AST_SET_LEFT_VALUE_STRING:
            clone.string_value = clone_string_value(orig.string_value);
            break;
        case AST_SET_LEFT_VALUE_VARIABLE:
            clone.variable_value = clone_attr_var(orig.variable_value);
            break;
        default:
            switch_default_error("Invalid set left value type");
            break;
    }
    return clone;
}

static struct betree_integer_list* clone_integer_list(struct betree_integer_list* list)
{
    struct betree_integer_list* clone = make_integer_list();
    clone->count = list->count;
    clone->integers = malloc(sizeof(*clone->integers) * clone->count);
    for(size_t i = 0; i < list->count; i++) {
        clone->integers[i] = list->integers[i];
    }
    return clone;
}

static struct betree_string_list* clone_string_list(struct betree_string_list* list)
{
    struct betree_string_list* clone = make_string_list();
    clone->count = list->count;
    clone->strings = malloc(sizeof(*clone->strings) * clone->count);
    for(size_t i = 0; i < list->count; i++) {
        struct string_value string_clone = clone_string_value(list->strings[i]);
        clone->strings[i] = string_clone;
    }
    return clone;
}

static struct set_right_value clone_set_right_value(struct set_right_value orig)
{
    struct set_right_value clone = { .value_type = orig.value_type };
    switch(orig.value_type) {
        case AST_SET_RIGHT_VALUE_INTEGER_LIST:
            clone.integer_list_value = clone_integer_list(orig.integer_list_value);
            break;
        case AST_SET_RIGHT_VALUE_STRING_LIST:
            clone.string_list_value = clone_string_list(orig.string_list_value);
            break;
        case AST_SET_RIGHT_VALUE_VARIABLE:
            clone.variable_value = clone_attr_var(orig.variable_value);
            break;
        default:
            switch_default_error("Invalid set right value type");
            break;
    }
    return clone;
}

static struct ast_node* clone_set(betree_pred_t id, struct ast_set_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_SET_EXPR;
    clone->set_expr.op = orig.op;
    clone->set_expr.left_value = clone_set_left_value(orig.left_value);
    clone->set_expr.right_value = clone_set_right_value(orig.right_value);
    return clone;
}

static struct list_value clone_list_value(struct list_value orig)
{
    struct list_value clone = { .value_type = orig.value_type };
    switch(orig.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST:
            clone.integer_list_value = clone_integer_list(orig.integer_list_value);
            break;
        case AST_LIST_VALUE_STRING_LIST:
            clone.string_list_value = clone_string_list(orig.string_list_value);
            break;
        default:
            switch_default_error("Invalid list value type");
            break;
    }
    return clone;
}

static struct ast_node* clone_list(betree_pred_t id, struct ast_list_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_LIST_EXPR;
    clone->list_expr.attr_var = clone_attr_var(orig.attr_var);
    clone->list_expr.op = orig.op;
    clone->list_expr.value = clone_list_value(orig.value);
    return clone;
}

static struct ast_node* clone_special(betree_pred_t id, struct ast_special_expr orig)
{
    struct ast_node* clone = ast_node_create();
    clone->id = id;
    clone->type = AST_TYPE_SPECIAL_EXPR;
    clone->special_expr.type = orig.type;
    switch(orig.type) {
        case AST_SPECIAL_FREQUENCY:
            clone->special_expr.frequency.attr_var = clone_attr_var(orig.frequency.attr_var);
            clone->special_expr.frequency.length = orig.frequency.length;
            clone->special_expr.frequency.ns = clone_string_value(orig.frequency.ns);
            clone->special_expr.frequency.op = orig.frequency.op;
            clone->special_expr.frequency.type = orig.frequency.type;
            clone->special_expr.frequency.value = orig.frequency.value;
            break;
        case AST_SPECIAL_SEGMENT:
            clone->special_expr.segment.attr_var = clone_attr_var(orig.segment.attr_var);
            clone->special_expr.segment.has_variable = orig.segment.has_variable;
            clone->special_expr.segment.op = orig.segment.op;
            clone->special_expr.segment.seconds = orig.segment.seconds;
            clone->special_expr.segment.segment_id = orig.segment.segment_id;
            break;
        case AST_SPECIAL_GEO:
            clone->special_expr.geo.has_radius = orig.geo.has_radius;
            clone->special_expr.geo.latitude = orig.geo.latitude;
            clone->special_expr.geo.longitude = orig.geo.longitude;
            clone->special_expr.geo.op = orig.geo.op;
            clone->special_expr.geo.radius = orig.geo.radius;
            break;
        case AST_SPECIAL_STRING:
            clone->special_expr.string.attr_var = clone_attr_var(orig.string.attr_var);
            clone->special_expr.string.op = orig.string.op;
            clone->special_expr.string.pattern = strdup(orig.string.pattern);
            break;
        default:
            switch_default_error("Invalid special expr type");
            break;
    }
    return clone;
}

struct ast_node* clone_node(const struct ast_node* node)
{
    struct ast_node* clone = NULL;
    switch(node->type) {
        case AST_TYPE_COMPARE_EXPR:
            clone = clone_compare(node->id, node->compare_expr);
            break;
        case AST_TYPE_EQUALITY_EXPR:
            clone = clone_equality(node->id, node->equality_expr);
            break;
        case AST_TYPE_BOOL_EXPR:
            clone = clone_bool(node->id, node->bool_expr);
            break;
        case AST_TYPE_SET_EXPR:
            clone = clone_set(node->id, node->set_expr);
            break;
        case AST_TYPE_LIST_EXPR:
            clone = clone_list(node->id, node->list_expr);
            break;
        case AST_TYPE_SPECIAL_EXPR:
            clone = clone_special(node->id, node->special_expr);
            break;
        default:
            switch_default_error("Invalid node type");
            break;
    }
    return clone;
}
