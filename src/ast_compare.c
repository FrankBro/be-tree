#include "ast_compare.h"

#include "ast.h"
#include "value.h"

static int cmp_integer_list(struct betree_integer_list* l1, struct betree_integer_list* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    if(l1->count < l2->count) {
        return -1;
    }
    for(size_t i = 0; i < l1->count; i++) {
        if(l1->integers[i] > l2->integers[i]) {
            return 1;
        }
        if(l1->integers[i] < l2->integers[i]) {
            return -1;
        }
    }
    return 0;
}

static int cmp_string_list(struct betree_string_list* l1, struct betree_string_list* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    if(l1->count < l2->count) {
        return -1;
    }
    for(size_t i = 0; i < l1->count; i++) {
        if(l1->strings[i].str > l2->strings[i].str) {
            return 1;
        }
        if(l1->strings[i].str < l2->strings[i].str) {
            return -1;
        }
    }
    return 0;
}

static int cmp_integer_enum_list(struct betree_integer_enum_list* l1, struct betree_integer_enum_list* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    if(l1->count < l2->count) {
        return -1;
    }
    for(size_t i = 0; i < l1->count; i++) {
        if(l1->integers[i].ienum > l2->integers[i].ienum) {
            return 1;
        }
        if(l1->integers[i].ienum < l2->integers[i].ienum) {
            return -1;
        }
    }
    return 0;
}

static int compare_expr_cmp(struct ast_compare_expr* c1, struct ast_compare_expr* c2)
{
    if(c1->op > c2->op) {
        return 1;
    }
    if(c1->op < c2->op) {
        return -1;
    }
    if(c1->attr_var.var > c2->attr_var.var) {
        return 1;
    }
    if(c1->attr_var.var < c2->attr_var.var) {
        return -1;
    }
    if(c1->value.value_type > c2->value.value_type) {
        return 1;
    }
    if(c1->value.value_type < c2->value.value_type) {
        return -1;
    }
    switch(c1->value.value_type) {
        case AST_COMPARE_VALUE_INTEGER:
            if(c1->value.integer_value > c2->value.integer_value) {
                return 1;
            }
            if(c1->value.integer_value < c2->value.integer_value) {
                return -1;
            }
            return 0;
        case AST_COMPARE_VALUE_FLOAT:
            if(c1->value.float_value > c2->value.float_value) {
                return 1;
            }
            if(c1->value.float_value < c2->value.float_value) {
                return -1;
            }
            return 0;
        default: abort();
    }
}

static int equality_expr_cmp(struct ast_equality_expr* e1, struct ast_equality_expr* e2)
{
    if(e1->op > e2->op) {
        return 1;
    }
    if(e1->op < e2->op) {
        return -1;
    }
    if(e1->attr_var.var > e2->attr_var.var) {
        return 1;
    }
    if(e1->attr_var.var < e2->attr_var.var) {
        return -1;
    }
    if(e1->value.value_type > e2->value.value_type) {
        return 1;
    }
    if(e1->value.value_type < e2->value.value_type) {
        return -1;
    }
    switch(e1->value.value_type) {
        case AST_EQUALITY_VALUE_INTEGER_ENUM:
            if(e1->value.integer_enum_value.ienum > e2->value.integer_enum_value.ienum) {
                return 1;
            }
            if(e1->value.integer_enum_value.ienum < e2->value.integer_enum_value.ienum) {
                return -1;
            }
            return 0;
        case AST_EQUALITY_VALUE_INTEGER:
            if(e1->value.integer_value > e2->value.integer_value) {
                return 1;
            }
            if(e1->value.integer_value < e2->value.integer_value) {
                return -1;
            }
            return 0;
        case AST_EQUALITY_VALUE_FLOAT:
            if(e1->value.float_value > e2->value.float_value) {
                return 1;
            }
            if(e1->value.float_value < e2->value.float_value) {
                return -1;
            }
            return 0;
        case AST_EQUALITY_VALUE_STRING:
            if(e1->value.string_value.str > e2->value.string_value.str) {
                return 1;
            }
            if(e1->value.string_value.str < e2->value.string_value.str) {
                return -1;
            }
            return 0;
        default: abort();
    }
}

static int bool_expr_cmp(struct ast_bool_expr* b1, struct ast_bool_expr* b2)
{
    if(b1->op > b2->op) {
        return 1;
    }
    if(b1->op < b2->op) {
        return -1;
    }
    switch(b1->op) {
        case AST_BOOL_OR:
        case AST_BOOL_AND: {
            // Global id will be assigned
            if(b1->binary.lhs->global_id > b2->binary.lhs->global_id) {
                return 1;
            }
            if(b1->binary.lhs->global_id < b2->binary.lhs->global_id) {
                return -1;
            }
            if(b1->binary.rhs->global_id > b2->binary.rhs->global_id) {
                return 1;
            }
            if(b1->binary.rhs->global_id < b2->binary.rhs->global_id) {
                return -1;
            }
            return 0;
        }
        case AST_BOOL_NOT:
            // Global id will be assigned
            if(b1->unary.expr->global_id > b2->unary.expr->global_id) {
                return 1;
            }
            if(b1->unary.expr->global_id < b2->unary.expr->global_id) {
                return -1;
            }
            return 0;
        case AST_BOOL_VARIABLE:
            if(b1->variable.var > b2->variable.var) {
                return 1;
            }
            if(b1->variable.var < b2->variable.var) {
                return -1;
            }
            return 0;
        case AST_BOOL_LITERAL:
            if(b1->literal > b2->literal) {
                return 1;
            }
            if(b1->literal < b2->literal) {
                return -1;
            }
            return 0;
        default: abort();
    }
}

static int set_expr_cmp(struct ast_set_expr* s1, struct ast_set_expr* s2)
{
    if(s1->op > s2->op) {
        return 1;
    }
    if(s1->op < s2->op) {
        return -1;
    }
    if(s1->left_value.value_type > s2->left_value.value_type) {
        return 1;
    }
    if(s1->left_value.value_type < s2->left_value.value_type) {
        return -1;
    }
    if(s1->right_value.value_type > s2->right_value.value_type) {
        return 1;
    }
    if(s1->right_value.value_type < s2->right_value.value_type) {
        return -1;
    }
    switch(s1->left_value.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            if(s1->left_value.integer_value > s2->left_value.integer_value) {
                return 1;
            }
            if(s1->left_value.integer_value < s2->left_value.integer_value) {
                return -1;
            }
            break;
        case AST_SET_LEFT_VALUE_STRING:
            if(s1->left_value.string_value.str > s2->left_value.string_value.str) {
                return 1;
            }
            if(s1->left_value.string_value.str < s2->left_value.string_value.str) {
                return -1;
            }
            break;
        case AST_SET_LEFT_VALUE_VARIABLE:
            if(s1->left_value.variable_value.var > s2->left_value.variable_value.var) {
                return 1;
            }
            if(s1->left_value.variable_value.var < s2->left_value.variable_value.var) {
                return -1;
            }
            break;
        default: abort();
    }
    switch(s1->right_value.value_type) {
        case AST_SET_RIGHT_VALUE_INTEGER_LIST:
            return cmp_integer_list(s1->right_value.integer_list_value, s2->right_value.integer_list_value);
        case AST_SET_RIGHT_VALUE_STRING_LIST:
            return cmp_string_list(s1->right_value.string_list_value, s2->right_value.string_list_value);
        case AST_SET_RIGHT_VALUE_VARIABLE:
            if(s1->right_value.variable_value.var > s2->right_value.variable_value.var) {
                return 1;
            }
            if(s1->right_value.variable_value.var < s2->right_value.variable_value.var) {
                return -1;
            }
            return 0;
        case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
            return cmp_integer_enum_list(s1->right_value.integer_enum_list_value, s2->right_value.integer_enum_list_value);
        default: abort();
    }
}

static int list_expr_cmp(struct ast_list_expr* l1, struct ast_list_expr* l2)
{
    if(l1->op > l2->op) {
        return 1;
    }
    if(l1->op < l2->op) {
        return -1;
    }
    if(l1->attr_var.var > l2->attr_var.var) {
        return 1;
    }
    if(l1->attr_var.var < l2->attr_var.var) {
        return -1;
    }
    if(l1->value.value_type > l2->value.value_type) {
        return 1;
    }
    if(l1->value.value_type < l2->value.value_type) {
        return -1;
    }
    switch(l1->value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST:
            return cmp_integer_list(l1->value.integer_list_value, l2->value.integer_list_value);
        case AST_LIST_VALUE_STRING_LIST:
            return cmp_string_list(l1->value.string_list_value, l2->value.string_list_value);
        default: abort();
    }
}

static int special_expr_cmp(struct ast_special_expr* s1, struct ast_special_expr* s2)
{
    if(s1->type > s2->type) {
        return 1;
    }
    if(s1->type < s2->type) {
        return -1;
    }
    switch(s1->type) {
        case AST_SPECIAL_FREQUENCY:
            if(s1->frequency.op > s2->frequency.op) {
                return 1;
            }
            if(s1->frequency.op < s2->frequency.op) {
                return -1;
            }
            if(s1->frequency.attr_var.var > s2->frequency.attr_var.var) {
                return 1;
            }
            if(s1->frequency.attr_var.var < s2->frequency.attr_var.var) {
                return -1;
            }
            if(s1->frequency.id > s2->frequency.id) {
                return 1;
            }
            if(s1->frequency.id < s2->frequency.id) {
                return -1;
            }
            if(s1->frequency.length > s2->frequency.length) {
                return 1;
            }
            if(s1->frequency.length < s2->frequency.length) {
                return -1;
            }
            if(s1->frequency.ns.str > s2->frequency.ns.str) {
                return 1;
            }
            if(s1->frequency.ns.str < s2->frequency.ns.str) {
                return -1;
            }
            if(s1->frequency.type > s2->frequency.type) {
                return 1;
            }
            if(s1->frequency.type < s2->frequency.type) {
                return -1;
            }
            if(s1->frequency.value > s2->frequency.value) {
                return 1;
            }
            if(s1->frequency.value < s2->frequency.value) {
                return -1;
            }
            return 0;
        case AST_SPECIAL_SEGMENT:
            if(s1->segment.op > s2->segment.op) {
                return 1;
            }
            if(s1->segment.op < s2->segment.op) {
                return -1;
            }
            if(s1->segment.has_variable > s2->segment.has_variable) {
                return 1;
            }
            if(s1->segment.has_variable < s2->segment.has_variable) {
                return -1;
            }
            if(s1->segment.attr_var.var > s2->segment.attr_var.var) {
                return 1;
            }
            if(s1->segment.attr_var.var < s2->segment.attr_var.var) {
                return -1;
            }
            if(s1->segment.seconds > s2->segment.seconds) {
                return 1;
            }
            if(s1->segment.seconds < s2->segment.seconds) {
                return -1;
            }
            if(s1->segment.segment_id > s2->segment.segment_id) {
                return 1;
            }
            if(s1->segment.segment_id < s2->segment.segment_id) {
                return -1;
            }
            return 0;
        case AST_SPECIAL_GEO:
            if(s1->geo.op > s2->geo.op) {
                return 1;
            }
            if(s1->geo.op < s2->geo.op) {
                return -1;
            }
            if(s1->geo.has_radius > s2->geo.has_radius) {
                return 1;
            }
            if(s1->geo.has_radius < s2->geo.has_radius) {
                return -1;
            }
            if(s1->geo.latitude_var.var > s2->geo.latitude_var.var) {
                return 1;
            }
            if(s1->geo.latitude_var.var < s2->geo.latitude_var.var) {
                return -1;
            }
            if(s1->geo.longitude_var.var > s2->geo.longitude_var.var) {
                return 1;
            }
            if(s1->geo.longitude_var.var < s2->geo.longitude_var.var) {
                return -1;
            }
            if(s1->geo.latitude > s2->geo.latitude) {
                return 1;
            }
            if(s1->geo.latitude < s2->geo.latitude) {
                return -1;
            }
            if(s1->geo.longitude > s2->geo.longitude) {
                return 1;
            }
            if(s1->geo.longitude < s2->geo.longitude) {
                return -1;
            }
            if(s1->geo.radius > s2->geo.radius) {
                return 1;
            }
            if(s1->geo.radius < s2->geo.radius) {
                return -1;
            }
            return 0;
        case AST_SPECIAL_STRING:
            if(s1->string.op > s2->string.op) {
                return 1;
            }
            if(s1->string.op < s2->string.op) {
                return -1;
            }
            if(s1->string.attr_var.var > s2->string.attr_var.var) {
                return 1;
            }
            if(s1->string.attr_var.var < s2->string.attr_var.var) {
                return -1;
            }
            return strcmp(s1->string.pattern, s2->string.pattern);
        default: abort();
    }
}

static int is_null_expr_cmp(struct ast_is_null_expr* i1, struct ast_is_null_expr* i2)
{
    if(i1->attr_var.var > i2->attr_var.var) {
        return 1;
    }
    if(i1->attr_var.var < i2->attr_var.var) {
        return -1;
    }
    if(i1->type > i2->type) {
        return 1;
    }
    if(i1->type < i2->type) {
        return -1;
    }
    return 0;
}

int expr_cmp(const void* p1, const void* p2)
{
    struct ast_node *n1 = (struct ast_node*)p1;
    struct ast_node *n2 = (struct ast_node*)p2;

    if(n1->type > n2->type) {
        return 1;
    }
    if(n1->type < n2->type) {
        return -1;
    }
    switch(n1->type) {
        case AST_TYPE_COMPARE_EXPR: return compare_expr_cmp(&n1->compare_expr, &n2->compare_expr);
        case AST_TYPE_EQUALITY_EXPR: return equality_expr_cmp(&n1->equality_expr, &n2->equality_expr);
        case AST_TYPE_BOOL_EXPR: return bool_expr_cmp(&n1->bool_expr, &n2->bool_expr);
        case AST_TYPE_SET_EXPR: return set_expr_cmp(&n1->set_expr, &n2->set_expr);
        case AST_TYPE_LIST_EXPR: return list_expr_cmp(&n1->list_expr, &n2->list_expr);
        case AST_TYPE_SPECIAL_EXPR: return special_expr_cmp(&n1->special_expr, &n2->special_expr);
        case AST_TYPE_IS_NULL_EXPR: return is_null_expr_cmp(&n1->is_null_expr, &n2->is_null_expr);
        default: abort();
    }
}

