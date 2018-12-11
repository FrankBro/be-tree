#include <stdio.h>

#include "alloc.h"
#include "ast.h"
#include "clone.h"
#include "hashmap.h"
#include "jsw_rbtree.h"
#include "map.h"
#include "printer.h"
#include "utils.h"

void assign_pred(struct pred_map* pred_map, struct ast_node* node)
{
    if(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_NOT) {
        assign_pred(pred_map, node->bool_expr.unary.expr);
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
    }
    struct ast_node* find = jsw_rbfind(pred_map->m, node);
    if(find == NULL) {
        betree_pred_t global_id = pred_map->pred_count;
        pred_map->pred_count++;
        node->global_id = global_id;
        int ret = jsw_rbinsert(pred_map->m, node);
        if(ret == 0) {
            abort();
        }
    }
    else {
        node->global_id = find->global_id;
        if(find->memoize_id == INVALID_PRED) {
            betree_pred_t memoize_id = pred_map->memoize_count;
            pred_map->memoize_count++;
            find->memoize_id = memoize_id;
        }
        node->memoize_id = find->memoize_id;
    }
}

static exprmap_t* exprmap_new();

struct pred_map* make_pred_map()
{
    struct pred_map* pred_map = bcalloc(sizeof(*pred_map));
    if(pred_map == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    pred_map->pred_count = 0;
    pred_map->m = exprmap_new();
    return pred_map;
}

void free_pred_map(struct pred_map* pred_map)
{
    /*const char *key;*/
    /*map_iter_t iter = map_iter(&pred_map->m);*/

    /*while ((key = map_next(&pred_map->m, &iter))) {*/
        /*bfree((char*)key);*/
    /*}*/
    bfree(pred_map);
}

static int cmp_integer_list(struct betree_integer_list* l1, struct betree_integer_list* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    else if(l1->count < l2->count) {
        return -1;
    }
    else {
        for(size_t i = 0; i < l1->count; i++) {
            if(l1->integers[i] > l2->integers[i]) {
                return 1;
            }
            else if(l1->integers[i] < l2->integers[i]) {
                return -1;
            }
        }
        return 0;
    }
}

static int cmp_string_list(struct betree_string_list* l1, struct betree_string_list* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    else if(l1->count < l2->count) {
        return -1;
    }
    else {
        for(size_t i = 0; i < l1->count; i++) {
            if(l1->strings[i].str > l2->strings[i].str) {
                return 1;
            }
            else if(l1->strings[i].str < l2->strings[i].str) {
                return -1;
            }
        }
        return 0;
    }
}

static int cmp_integer_list_enum(struct betree_integer_list_enum* l1, struct betree_integer_list_enum* l2)
{
    if(l1->count > l2->count) {
        return 1;
    }
    else if(l1->count < l2->count) {
        return -1;
    }
    else {
        for(size_t i = 0; i < l1->count; i++) {
            if(l1->integers[i].ienum > l2->integers[i].ienum) {
                return 1;
            }
            else if(l1->integers[i].ienum < l2->integers[i].ienum) {
                return -1;
            }
        }
        return 0;
    }
}

static int expr_cmp(const void* p1, const void* p2)
{
    struct ast_node *n1 = (struct ast_node*)p1;
    struct ast_node *n2 = (struct ast_node*)p2;

    if(n1->type > n2->type) {
        return 1;
    }
    else if(n1->type < n2->type) {
        return -1;
    }
    else {
        switch(n1->type) {
            case AST_TYPE_COMPARE_EXPR: {
                struct ast_compare_expr* c1 = &n1->compare_expr;
                struct ast_compare_expr* c2 = &n2->compare_expr;
                if(c1->op > c2->op) {
                    return 1;
                }
                else if(c1->op < c2->op) {
                    return -1;
                }
                else {
                    if(c1->attr_var.var > c2->attr_var.var) {
                        return 1;
                    }
                    else if(c1->attr_var.var < c2->attr_var.var) {
                        return -1;
                    }
                    else {
                        if(c1->value.value_type > c2->value.value_type) {
                            return 1;
                        }
                        else if(c1->value.value_type < c2->value.value_type) {
                            return -1;
                        }
                        else {
                            switch(c1->value.value_type) {
                                case AST_COMPARE_VALUE_INTEGER:
                                    if(c1->value.integer_value > c2->value.integer_value) {
                                        return 1;
                                    }
                                    else if(c1->value.integer_value < c2->value.integer_value) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                case AST_COMPARE_VALUE_FLOAT:
                                    if(c1->value.float_value > c2->value.float_value) {
                                        return 1;
                                    }
                                    else if(c1->value.float_value < c2->value.float_value) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                default: abort();
                            }
                        }
                    }
                }
            }
            case AST_TYPE_EQUALITY_EXPR: {
                struct ast_equality_expr* e1 = &n1->equality_expr;
                struct ast_equality_expr* e2 = &n2->equality_expr;
                if(e1->op > e2->op) {
                    return 1;
                }
                else if(e1->op < e2->op) {
                    return -1;
                }
                else {
                    if(e1->attr_var.var > e2->attr_var.var) {
                        return 1;
                    }
                    else if(e1->attr_var.var < e2->attr_var.var) {
                        return -1;
                    }
                    else {
                        if(e1->value.value_type > e2->value.value_type) {
                            return 1;
                        }
                        else if(e1->value.value_type < e2->value.value_type) {
                            return -1;
                        }
                        else {
                            switch(e1->value.value_type) {
                                case AST_EQUALITY_VALUE_INTEGER_ENUM:
                                    if(e1->value.integer_enum_value.ienum > e2->value.integer_enum_value.ienum) {
                                        return 1;
                                    }
                                    else if(e1->value.integer_enum_value.ienum < e2->value.integer_enum_value.ienum) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                case AST_EQUALITY_VALUE_INTEGER:
                                    if(e1->value.integer_value > e2->value.integer_value) {
                                        return 1;
                                    }
                                    else if(e1->value.integer_value < e2->value.integer_value) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                case AST_EQUALITY_VALUE_FLOAT:
                                    if(e1->value.float_value > e2->value.float_value) {
                                        return 1;
                                    }
                                    else if(e1->value.float_value < e2->value.float_value) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                case AST_EQUALITY_VALUE_STRING:
                                    if(e1->value.string_value.str > e2->value.string_value.str) {
                                        return 1;
                                    }
                                    else if(e1->value.string_value.str < e2->value.string_value.str) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                default: abort();
                            }
                        }
                    }
                }
            }
            case AST_TYPE_BOOL_EXPR: {
                struct ast_bool_expr* b1 = &n1->bool_expr;
                struct ast_bool_expr* b2 = &n2->bool_expr;
                if(b1->op > b2->op) {
                    return 1;
                }
                else if(b1->op < b2->op) {
                    return -1;
                }
                else {
                    switch(b1->op) {
                        case AST_BOOL_OR:
                        case AST_BOOL_AND: {
                            // Global id will be assigned
                            if(b1->binary.lhs->global_id > b2->binary.lhs->global_id) {
                                return 1;
                            }
                            else if(b1->binary.lhs->global_id < b2->binary.lhs->global_id) {
                                return -1;
                            }
                            else {
                                if(b1->binary.rhs->global_id > b2->binary.rhs->global_id) {
                                    return 1;
                                }
                                else if(b1->binary.rhs->global_id < b2->binary.rhs->global_id) {
                                    return -1;
                                }
                                else {
                                    return 0;
                                }
                            }
                        }
                        case AST_BOOL_NOT:
                            // Global id will be assigned
                            if(b1->unary.expr->global_id > b2->unary.expr->global_id) {
                                return 1;
                            }
                            else if(b1->unary.expr->global_id < b2->unary.expr->global_id) {
                                return -1;
                            }
                            else {
                                return 0;
                            }
                        case AST_BOOL_VARIABLE:
                            if(b1->variable.var > b2->variable.var) {
                                return 1;
                            }
                            else if(b1->variable.var < b2->variable.var) {
                                return -1;
                            }
                            else {
                                return 0;
                            }
                        case AST_BOOL_LITERAL:
                            if(b1->literal > b2->literal) {
                                return 1;
                            }
                            else if(b1->literal < b2->literal) {
                                return -1;
                            }
                            else {
                                return 0;
                            }
                        default: abort();
                    }
                }
            }
            case AST_TYPE_SET_EXPR: {
                struct ast_set_expr* s1 = &n1->set_expr;
                struct ast_set_expr* s2 = &n2->set_expr;
                if(s1->op > s2->op) {
                    return 1;
                }
                else if(s1->op < s2->op) {
                    return -1;
                }
                else {
                    if(s1->left_value.value_type > s2->left_value.value_type) {
                        return 1;
                    }
                    else if(s1->left_value.value_type < s2->left_value.value_type) {
                        return -1;
                    }
                    else {
                        if(s1->right_value.value_type > s2->right_value.value_type) {
                            return 1;
                        }
                        else if(s1->right_value.value_type < s2->right_value.value_type) {
                            return -1;
                        }
                        else {
                            switch(s1->left_value.value_type) {
                                case AST_SET_LEFT_VALUE_INTEGER:
                                    if(s1->left_value.integer_value > s2->left_value.integer_value) {
                                        return 1;
                                    }
                                    else if(s1->left_value.integer_value < s2->left_value.integer_value) {
                                        return -1;
                                    }
                                    break;
                                case AST_SET_LEFT_VALUE_STRING:
                                    if(s1->left_value.string_value.str > s2->left_value.string_value.str) {
                                        return 1;
                                    }
                                    else if(s1->left_value.string_value.str < s2->left_value.string_value.str) {
                                        return -1;
                                    }
                                    break;
                                case AST_SET_LEFT_VALUE_VARIABLE:
                                    if(s1->left_value.variable_value.var > s2->left_value.variable_value.var) {
                                        return 1;
                                    }
                                    else if(s1->left_value.variable_value.var < s2->left_value.variable_value.var) {
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
                                    else if(s1->right_value.variable_value.var < s2->right_value.variable_value.var) {
                                        return -1;
                                    }
                                    else {
                                        return 0;
                                    }
                                case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
                                    return cmp_integer_list_enum(s1->right_value.integer_list_enum_value, s2->right_value.integer_list_enum_value);
                                default: abort();
                            }
                        }
                    }
                }
            }
            case AST_TYPE_LIST_EXPR: {
                struct ast_list_expr* l1 = &n1->list_expr;
                struct ast_list_expr* l2 = &n2->list_expr;
                if(l1->op > l2->op) {
                    return 1;
                }
                else if(l1->op < l2->op) {
                    return -1;
                }
                else {
                    if(l1->attr_var.var > l2->attr_var.var) {
                        return 1;
                    }
                    else if(l1->attr_var.var < l2->attr_var.var) {
                        return -1;
                    }
                    else {
                        if(l1->value.value_type > l2->value.value_type) {
                            return 1;
                        }
                        else if(l1->value.value_type < l2->value.value_type) {
                            return -1;
                        }
                        else {
                            switch(l1->value.value_type) {
                                case AST_LIST_VALUE_INTEGER_LIST:
                                    return cmp_integer_list(l1->value.integer_list_value, l2->value.integer_list_value);
                                case AST_LIST_VALUE_STRING_LIST:
                                    return cmp_string_list(l1->value.string_list_value, l2->value.string_list_value);
                                default: abort();
                            }
                        }
                    }
                }
            }
            case AST_TYPE_SPECIAL_EXPR: {
                struct ast_special_expr* s1 = &n1->special_expr;
                struct ast_special_expr* s2 = &n2->special_expr;
                if(s1->type > s2->type) {
                    return 1;
                }
                else if(s1->type < s2->type) {
                    return -1;
                }
                else {
                    switch(s1->type) {
                        case AST_SPECIAL_FREQUENCY:
                            if(s1->frequency.op > s2->frequency.op) {
                                return 1;
                            }
                            else if(s1->frequency.op < s2->frequency.op) {
                                return -1;
                            }
                            else {
                                if(s1->frequency.attr_var.var > s2->frequency.attr_var.var) {
                                    return 1;
                                }
                                else if(s1->frequency.attr_var.var < s2->frequency.attr_var.var) {
                                    return -1;
                                }
                                else {
                                    if(s1->frequency.id > s2->frequency.id) {
                                        return 1;
                                    }
                                    else if(s1->frequency.id < s2->frequency.id) {
                                        return -1;
                                    }
                                    else {
                                        if(s1->frequency.length > s2->frequency.length) {
                                            return 1;
                                        }
                                        else if(s1->frequency.length < s2->frequency.length) {
                                            return -1;
                                        }
                                        else {
                                            if(s1->frequency.ns.str > s2->frequency.ns.str) {
                                                return 1;
                                            }
                                            else if(s1->frequency.ns.str < s2->frequency.ns.str) {
                                                return -1;
                                            }
                                            else {
                                                if(s1->frequency.type > s2->frequency.type) {
                                                    return 1;
                                                }
                                                else if(s1->frequency.type < s2->frequency.type) {
                                                    return -1;
                                                }
                                                else {
                                                    if(s1->frequency.value > s2->frequency.value) {
                                                        return 1;
                                                    }
                                                    else if(s1->frequency.value < s2->frequency.value) {
                                                        return -1;
                                                    }
                                                    else {
                                                        return 0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        case AST_SPECIAL_SEGMENT:
                            if(s1->segment.op > s2->segment.op) {
                                return 1;
                            }
                            else if(s1->segment.op < s2->segment.op) {
                                return -1;
                            }
                            else {
                                if(s1->segment.has_variable > s2->segment.has_variable) {
                                    return 1;
                                }
                                else if(s1->segment.has_variable < s2->segment.has_variable) {
                                    return -1;
                                }
                                else {
                                    if(s1->segment.attr_var.var > s2->segment.attr_var.var) {
                                        return 1;
                                    }
                                    else if(s1->segment.attr_var.var < s2->segment.attr_var.var) {
                                        return -1;
                                    }
                                    else {
                                        if(s1->segment.seconds > s2->segment.seconds) {
                                            return 1;
                                        }
                                        else if(s1->segment.seconds < s2->segment.seconds) {
                                            return -1;
                                        }
                                        else {
                                            if(s1->segment.segment_id > s2->segment.segment_id) {
                                                return 1;
                                            }
                                            else if(s1->segment.segment_id < s2->segment.segment_id) {
                                                return -1;
                                            }
                                            else {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                            }
                        case AST_SPECIAL_GEO:
                            if(s1->geo.op > s2->geo.op) {
                                return 1;
                            }
                            else if(s1->geo.op < s2->geo.op) {
                                return -1;
                            }
                            else {
                                if(s1->geo.has_radius > s2->geo.has_radius) {
                                    return 1;
                                }
                                else if(s1->geo.has_radius < s2->geo.has_radius) {
                                    return -1;
                                }
                                else {
                                    if(s1->geo.latitude_var.var > s2->geo.latitude_var.var) {
                                        return 1;
                                    }
                                    else if(s1->geo.latitude_var.var < s2->geo.latitude_var.var) {
                                        return -1;
                                    }
                                    else {
                                        if(s1->geo.longitude_var.var > s2->geo.longitude_var.var) {
                                            return 1;
                                        }
                                        else if(s1->geo.longitude_var.var < s2->geo.longitude_var.var) {
                                            return -1;
                                        }
                                        else {
                                            if(s1->geo.latitude > s2->geo.latitude) {
                                                return 1;
                                            }
                                            else if(s1->geo.latitude < s2->geo.latitude) {
                                                return -1;
                                            }
                                            else {
                                                if(s1->geo.longitude > s2->geo.longitude) {
                                                    return 1;
                                                }
                                                else if(s1->geo.longitude < s2->geo.longitude) {
                                                    return -1;
                                                }
                                                else {
                                                    if(s1->geo.radius > s2->geo.radius) {
                                                        return 1;
                                                    }
                                                    else if(s1->geo.radius < s2->geo.radius) {
                                                        return -1;
                                                    }
                                                    else {
                                                        return 0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        case AST_SPECIAL_STRING:
                            if(s1->string.op > s2->string.op) {
                                return 1;
                            }
                            else if(s1->string.op < s2->string.op) {
                                return -1;
                            }
                            else {
                                if(s1->string.attr_var.var > s2->string.attr_var.var) {
                                    return 1;
                                }
                                else if(s1->string.attr_var.var < s2->string.attr_var.var) {
                                    return -1;
                                }
                                else {
                                    return strcmp(s1->string.pattern, s2->string.pattern);
                                }
                            }
                        default: abort();
                    }
                }
            }
            case AST_TYPE_IS_NULL_EXPR: {
                struct ast_is_null_expr* i1 = &n1->is_null_expr;
                struct ast_is_null_expr* i2 = &n2->is_null_expr;
                if(i1->attr_var.var > i2->attr_var.var) {
                    return 1;
                }
                else if(i1->attr_var.var < i2->attr_var.var) {
                    return -1;
                }
                else {
                    if(i1->type > i2->type) {
                        return 1;
                    }
                    else if(i1->type < i2->type) {
                        return -1;
                    }
                    else {
                        return 0;
                    }
                }
            }
            default: abort();
        }
    }
}

static exprmap_t* exprmap_new()
{
    jsw_rbtree_t* rbtree;
    rbtree = jsw_rbnew(expr_cmp);

    return rbtree;
}

