#include <stdio.h>

#include "ast.h"
#include "printer.h"
#include "utils.h"

static const char* compare_value_to_string(struct compare_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_COMPARE_VALUE_INTEGER:
            if(asprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_COMPARE_VALUE_FLOAT:
            if(asprintf(&expr, "%.2f", value.float_value) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid compare value type");
            break;
    }
    return expr;
}

static const char* compare_op_to_string(enum ast_compare_e op)
{
    switch(op) {
        case AST_COMPARE_LT:
            return "<";
        case AST_COMPARE_LE:
            return "<=";
        case AST_COMPARE_GT:
            return ">";
        case AST_COMPARE_GE:
            return ">=";
        default:
            switch_default_error("Invalid compare operation");
            return NULL;
    }
}

static const char* equality_value_to_string(struct equality_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_EQUALITY_VALUE_INTEGER:
            if(asprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_EQUALITY_VALUE_FLOAT:
            if(asprintf(&expr, "%.2f", value.float_value) < 0) {
                abort();
            }
            break;
        case AST_EQUALITY_VALUE_STRING:
            if(asprintf(&expr, "\"%s\"", value.string_value.string) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid equality value type");
            break;
    }
    return expr;
}

static const char* equality_op_to_string(enum ast_equality_e op)
{
    switch(op) {
        case AST_EQUALITY_EQ:
            return "=";
        case AST_EQUALITY_NE:
            return "<>";
        default:
            switch_default_error("Invalid equality operation");
            return NULL;
    }
}

static const char* set_left_value_to_string(struct set_left_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            if(asprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_SET_LEFT_VALUE_STRING:
            if(asprintf(&expr, "\"%s\"", value.string_value.string) < 0) {
                abort();
            }
            break;
        case AST_SET_LEFT_VALUE_VARIABLE:
            if(asprintf(&expr, "%s", value.variable_value.attr) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid set left value type");
            break;
    }
    return expr;
}

static const char* set_right_value_to_string(struct set_right_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_RIGHT_VALUE_VARIABLE:
            if(asprintf(&expr, "%s", value.variable_value.attr) < 0) {
                abort();
            }
            break;
        case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
            const char* list = integer_list_value_to_string(value.integer_list_value);
            if(asprintf(&expr, "(%s)", list) < 0) {
                abort();
            }
            free((char*)list);
            break;
        }
        case AST_SET_RIGHT_VALUE_STRING_LIST: {
            const char* list = string_list_value_to_string(value.string_list_value);
            if(asprintf(&expr, "(%s)", list) < 0) {
                abort();
            }
            free((char*)list);
            break;
        }
        default:
            switch_default_error("Invalid set right value type");
            break;
    }
    return expr;
}

static const char* set_op_to_string(enum ast_set_e op)
{
    switch(op) {
        case AST_SET_IN:
            return "in";
        case AST_SET_NOT_IN:
            return "not in";
        default:
            switch_default_error("Invalid set op");
            return NULL;
    }
}

static const char* list_value_to_string(struct list_value value)
{
    char* list;
    switch(value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            const char* inner = integer_list_value_to_string(value.integer_list_value);
            if(asprintf(&list, "(%s)", inner) < 0) {
                abort();
            }
            free((char*)inner);
            break;
        }
        case AST_LIST_VALUE_STRING_LIST: {
            const char* inner = string_list_value_to_string(value.string_list_value);
            if(asprintf(&list, "(%s)", inner) < 0) {
                abort();
            }
            free((char*)inner);
            break;
        }
        default: {
            switch_default_error("Invalid list value type");
            break;
        }
    }
    return list;
}

static const char* list_op_to_string(enum ast_list_e op)
{
    switch(op) {
        case AST_LIST_ONE_OF: {
            return "one of";
        }
        case AST_LIST_NONE_OF: {
            return "none of";
        }
        case AST_LIST_ALL_OF: {
            return "all of";
        }
        default: {
            switch_default_error("Invalid list operation");
            return NULL;
        }
    }
}

char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    if(asprintf(&expr,
                           "within_frequency_cap(\"%s\", \"%s\", %ld, %zu)",
                           frequency_type_to_string(node->special_expr.frequency.type),
                           node->special_expr.frequency.ns.string,
                           node->special_expr.frequency.value,
                           node->special_expr.frequency.length)
                        < 0) {
                        abort();
                    }
                    return expr;
                case AST_SPECIAL_SEGMENT: {
                    const char* op;
                    switch(node->special_expr.segment.op) {
                        case AST_SPECIAL_SEGMENTWITHIN:
                            op = "segment_within";
                            break;
                        case AST_SPECIAL_SEGMENTBEFORE:
                            op = "segment_before";
                            break;
                        default:
                            switch_default_error("Invalid special segment op");
                            return NULL;
                    }
                    if(asprintf(&expr,
                           "%s(%s, %lu, %ld)",
                           op,
                           node->special_expr.segment.attr_var.attr,
                           node->special_expr.segment.segment_id,
                           node->special_expr.segment.seconds)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                case AST_SPECIAL_GEO: {
                    if(asprintf(&expr,
                           "geo_within_radius(%.2f, %.2f, %.2f)",
                           node->special_expr.geo.latitude,
                           node->special_expr.geo.longitude,
                           node->special_expr.geo.radius)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                case AST_SPECIAL_STRING: {
                    const char* op;
                    switch(node->special_expr.string.op) {
                        case AST_SPECIAL_CONTAINS:
                            op = "contains";
                            break;
                        case AST_SPECIAL_STARTSWITH:
                            op = "starts_with";
                            break;
                        case AST_SPECIAL_ENDSWITH:
                            op = "ends_with";
                            break;
                        default:
                            switch_default_error("Invalid special string op");
                            return NULL;
                    }
                    if(asprintf(&expr,
                           "%s(%s, \"%s\")",
                           op,
                           node->special_expr.string.attr_var.attr,
                           node->special_expr.string.pattern)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                default:
                    switch_default_error("Invalid special op");
                    return NULL;
            }
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_VARIABLE: {
                    if(asprintf(&expr, "%s", node->bool_expr.variable.attr) < 0) {
                        abort();
                    }
                    return expr;
                }
                case AST_BOOL_NOT: {
                    const char* a = ast_to_string(node->bool_expr.unary.expr);
                    if(asprintf(&expr, "(not (%s))", a) < 0) {
                        abort();
                    }
                    free((char*)a);
                    return expr;
                }
                case AST_BOOL_OR: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    if(asprintf(&expr, "((%s) or (%s))", a, b) < 0) {
                        abort();
                    }
                    free((char*)a);
                    free((char*)b);
                    return expr;
                }
                case AST_BOOL_AND: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    if(asprintf(&expr, "((%s) and (%s))", a, b) < 0) {
                        abort();
                    }
                    free((char*)a);
                    free((char*)b);
                    return expr;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return NULL;
                }
            }
        }
        case(AST_TYPE_SET_EXPR): {
            const char* left = set_left_value_to_string(node->set_expr.left_value);
            const char* right = set_right_value_to_string(node->set_expr.right_value);
            const char* op = set_op_to_string(node->set_expr.op);
            if(asprintf(&expr, "%s %s %s", left, op, right) < 0) {
                abort();
            }
            free((char*)left);
            free((char*)right);
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            const char* value = list_value_to_string(node->list_expr.value);
            const char* op = list_op_to_string(node->list_expr.op);
            if(asprintf(&expr, "%s %s %s", node->list_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            free((char*)value);
            return expr;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            const char* value = equality_value_to_string(node->equality_expr.value);
            const char* op = equality_op_to_string(node->equality_expr.op);
            if(asprintf(&expr, "%s %s %s", node->equality_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            free((char*)value);
            return expr;
        }
        case(AST_TYPE_COMPARE_EXPR): {
            const char* value = compare_value_to_string(node->compare_expr.value);
            const char* op = compare_op_to_string(node->compare_expr.op);
            if(asprintf(&expr, "%s %s %s", node->compare_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            free((char*)value);
            return expr;
        }
        default: {
            switch_default_error("Invalid expr type");
            return NULL;
        }
    }
}
