#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "betree.h"
#include "utils.h"

struct ast_node* ast_node_create()
{
    struct ast_node* node = calloc(1, sizeof(*node));
    if(node == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    return node;
}

struct ast_node* ast_numeric_compare_expr_create(const enum ast_numeric_compare_e op, const char* name, struct numeric_compare_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_NUMERIC_COMPARE_EXPR;
    node->numeric_compare_expr.op = op;
    node->numeric_compare_expr.name = strdup(name);
    node->numeric_compare_expr.variable_id = -1;
    node->numeric_compare_expr.value = value;
    return node;
}

struct ast_node* ast_equality_expr_create(const enum ast_equality_e op, const char* name, struct equality_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_EQUALITY_EXPR;
    node->equality_expr.op = op;
    node->equality_expr.name = strdup(name);
    node->equality_expr.variable_id = -1;
    node->equality_expr.value = value;
    return node;
}

struct ast_node* ast_bool_expr_create(const enum ast_bool_e op, const char* name)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_BOOL_EXPR;
    node->bool_expr.op = op;
    node->bool_expr.name = strdup(name);
    node->bool_expr.variable_id = -1;
    return node;
}

struct ast_node* ast_combi_expr_create(const enum ast_combi_e op, const struct ast_node* lhs, const struct ast_node* rhs)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_COMBI_EXPR;
    node->combi_expr.op = op;
    node->combi_expr.lhs = lhs;
    node->combi_expr.rhs = rhs;
    return node;
}

struct ast_node* ast_list_expr_create(const enum ast_list_e op, const char* name, struct integer_list list)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_LIST_EXPR;
    node->list_expr.op = op;
    node->list_expr.name = strdup(name);
    node->list_expr.variable_id = -1;
    node->list_expr.list = list;
    return node;
}

void free_ast_node(struct ast_node* node)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            free((char*)node->numeric_compare_expr.name);
            break;
        case AST_TYPE_EQUALITY_EXPR:
            free((char*)node->equality_expr.name);
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                free((char*)node->equality_expr.value.string_value.string);
            }
            break;
        case AST_TYPE_BOOL_EXPR:
            free((char*)node->bool_expr.name);
            break;
        case AST_TYPE_LIST_EXPR:
            free((char*)node->list_expr.name);
            free(node->list_expr.list.integers);
            break;
        case AST_TYPE_COMBI_EXPR:
            free_ast_node((struct ast_node*)node->combi_expr.lhs);
            free_ast_node((struct ast_node*)node->combi_expr.rhs);
            break;
    }
    free(node);
}

bool get_variable(betree_var_t variable_id, const struct event* event, struct value* value)
{
    for(size_t i=0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(variable_id == pred->variable_id) {
            *value = pred->value;
            return true;
        }
    }
    return false;
}

static void invalid_expr(const char* msg)
{
    fprintf(stderr, "%s", msg);
    abort();
}

bool integer_in_integer_list(int64_t integer, struct integer_list integer_list)
{
    for(size_t i = 0; i < integer_list.count; i++) {
        if(integer_list.integers[i] == integer) {
            return true;
        }
    }
    return false;
}

bool numeric_compare_value_matches(enum ast_numeric_compare_value_e a, enum value_e b) {
    return
        (a == AST_NUMERIC_COMPARE_VALUE_INTEGER && b == VALUE_I) ||
        (a == AST_NUMERIC_COMPARE_VALUE_FLOAT && b == VALUE_F);
}

bool equality_value_matches(enum ast_equality_value_e a, enum value_e b) {
    return
        (a == AST_EQUALITY_VALUE_INTEGER && b == VALUE_I) ||
        (a == AST_EQUALITY_VALUE_FLOAT && b == VALUE_F) ||
        (a == AST_EQUALITY_VALUE_STRING && b == VALUE_S);
}

bool match_node(const struct event* event, const struct ast_node *node)
{
    switch(node->type) {
        case AST_TYPE_BOOL_EXPR: {
            struct value variable;
            bool found = get_variable(node->bool_expr.variable_id, event, &variable);
            if(!found) {
                return false;
            }
            if(variable.value_type != VALUE_B) {
                invalid_expr("boolean expression with a variable that is not a boolean");
            }
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    return variable.bvalue;
                }
                case AST_BOOL_NOT: {
                    return !variable.bvalue;
                }
            }
        }
        case AST_TYPE_LIST_EXPR: {
            struct value variable;
            bool found = get_variable(node->list_expr.variable_id, event, &variable);
            if(!found) {
                return false;
            }
            switch(node->list_expr.op) {
                case AST_LISTOP_NOTIN: {
                    if(variable.value_type != VALUE_I) {
                        invalid_expr("variable is not an integer in `not in` expression");
                    }
                    bool result = !integer_in_integer_list(variable.ivalue, node->list_expr.list);
                    return result;
                }
                case AST_LISTOP_IN: {
                    if(variable.value_type != VALUE_I) {
                        invalid_expr("variable is not an integer in `in` expression");
                    }
                    bool result = integer_in_integer_list(variable.ivalue, node->list_expr.list);
                    return result;
                }
            }
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            struct value variable;
            bool found = get_variable(node->numeric_compare_expr.variable_id, event, &variable);
            if(!found) {
                return false;
            }
            if(!numeric_compare_value_matches(node->numeric_compare_expr.value.value_type, variable.value_type)) {
                invalid_expr("numeric compare value types do not match");
            }
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bool result = variable.ivalue < node->numeric_compare_expr.value.integer_value;
                            return result;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bool result = variable.fvalue < node->numeric_compare_expr.value.float_value;
                            return result;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_LE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bool result = variable.ivalue <= node->numeric_compare_expr.value.integer_value;
                            return result;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bool result = variable.fvalue <= node->numeric_compare_expr.value.float_value;
                            return result;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bool result = variable.ivalue > node->numeric_compare_expr.value.integer_value;
                            return result;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bool result = variable.fvalue > node->numeric_compare_expr.value.float_value;
                            return result;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bool result = variable.ivalue >= node->numeric_compare_expr.value.integer_value;
                            return result;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bool result = variable.fvalue >= node->numeric_compare_expr.value.float_value;
                            return result;
                        }
                    }
                }
            }
        }
        case AST_TYPE_EQUALITY_EXPR: {
            struct value variable;
            bool found = get_variable(node->equality_expr.variable_id, event, &variable);
            if(!found) {
                return false;
            }
            if(!equality_value_matches(node->equality_expr.value.value_type, variable.value_type)) {
                invalid_expr("equality value types do not match");
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bool result = variable.ivalue == node->equality_expr.value.integer_value;
                            return result;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bool result = feq(variable.fvalue, node->equality_expr.value.float_value);
                            return result;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            bool result = variable.svalue.str == node->equality_expr.value.string_value.str;
                            return result;
                        }
                    }
                }
                case AST_EQUALITY_NE: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bool result = variable.ivalue != node->equality_expr.value.integer_value;
                            return result;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bool result = fne(variable.fvalue, node->equality_expr.value.float_value);
                            return result;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            bool result = variable.svalue.str != node->equality_expr.value.string_value.str;
                            return result;
                        }
                    }
                }
            }
        }
        case AST_TYPE_COMBI_EXPR: {
            bool lhs = match_node(event, node->combi_expr.lhs);
            bool rhs = match_node(event, node->combi_expr.rhs);
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    return lhs && rhs;
                }
                case AST_COMBI_OR: {
                    return lhs || rhs;
                }
            }
        }
    }
}

void get_variable_bound(const struct attr_domain* domain, const struct ast_node* node, struct value_bound* bound)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_COMBI_EXPR: {
            get_variable_bound(domain, node->combi_expr.lhs, bound);
            get_variable_bound(domain, node->combi_expr.rhs, bound);
            return;
        }
        case AST_TYPE_LIST_EXPR: {
            switch(node->list_expr.op) {
                case AST_LISTOP_NOTIN: {
                    bound->imin = domain->bound.imin;
                    bound->imax = domain->bound.imax;
                    return;
                }
                case AST_LISTOP_IN: {
                    int64_t min = INT64_MAX, max = INT64_MIN;
                    for(size_t i = 0; i < node->list_expr.list.count; i++) {
                        int64_t value = node->list_expr.list.integers[i];
                        if(value < min) {
                            min = value;
                        }
                        if(value > max) {
                            max = value;
                        }
                    }
                    bound->imin = min(bound->imin, min);
                    bound->imax = max(bound->imax, max);
                    return;
                }
            }
        }
        case AST_TYPE_BOOL_EXPR: {
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    bound->bmin = min(bound->bmin, true);
                    bound->bmax = max(bound->bmax, true);
                    return;
                }
                case AST_BOOL_NOT: {
                    bound->bmin = min(bound->bmin, false);
                    bound->bmax = max(bound->bmax, false);
                    return;
                }
            }
        }
        case AST_TYPE_EQUALITY_EXPR: {
            if(domain->bound.value_type != bound->value_type || 
                !equality_value_matches(node->equality_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bound->imin = min(bound->imin, node->equality_expr.value.integer_value);
                            bound->imax = max(bound->imax, node->equality_expr.value.integer_value);
                            return;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->equality_expr.value.float_value);
                            bound->fmax = fmax(bound->fmax, node->equality_expr.value.float_value);
                            return;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            invalid_expr("Trying to get the bound of a string value");
                        }
                    }
                }
                case AST_EQUALITY_NE: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            invalid_expr("Trying to get the bound of a string value");
                        }
                    }
                }
            }
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            if(domain->bound.value_type != bound->value_type || 
                !numeric_compare_value_matches(node->numeric_compare_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
            }
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = max(bound->imax, node->numeric_compare_expr.value.integer_value - 1);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value - __DBL_EPSILON__);
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_LE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = max(bound->imax, node->numeric_compare_expr.value.integer_value);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value);
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = min(bound->imin, node->numeric_compare_expr.value.integer_value + 1);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value + __DBL_EPSILON__);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = min(bound->imin, node->numeric_compare_expr.value.integer_value);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                    }
                }
            }
        }
    }
}

void assign_variable_id(struct config* config, struct ast_node* node) 
{
    switch(node->type) {
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->numeric_compare_expr.name);
            node->numeric_compare_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->equality_expr.name);
            node->equality_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->bool_expr.name);
            node->bool_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_LIST_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->list_expr.name);
            node->list_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_COMBI_EXPR): {
            assign_variable_id(config, (struct ast_node*)node->combi_expr.lhs);
            assign_variable_id(config, (struct ast_node*)node->combi_expr.rhs);
            return;
        }
    }
}

void assign_str_id(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config, node->equality_expr.value.string_value.string);
                node->equality_expr.value.string_value.str = str_id;
            }
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            return;
        }
        case(AST_TYPE_LIST_EXPR): {
            return;
        }
        case(AST_TYPE_COMBI_EXPR): {
            assign_str_id(config, (struct ast_node*)node->combi_expr.lhs);
            assign_str_id(config, (struct ast_node*)node->combi_expr.rhs);
            return;
        }
    }
}

const char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case(AST_TYPE_COMBI_EXPR): {
            const char* a = ast_to_string(node->combi_expr.lhs);
            const char* b = ast_to_string(node->combi_expr.rhs);
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    asprintf(&expr, "%s && %s", a, b);
                    break;
                }
                case AST_COMBI_OR: {
                    asprintf(&expr, "%s || %s", a, b);
                    break;
                }
            }
            free((char*)a);
            free((char*)b);
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            const char* integer_list = integer_list_to_string(node->list_expr.list);
            switch(node->list_expr.op) {
                case AST_LISTOP_NOTIN: {
                    asprintf(&expr, "%s not in (%s)", node->list_expr.name, integer_list);
                }
                case AST_LISTOP_IN: {
                    asprintf(&expr, "%s in (%s)", node->list_expr.name, integer_list);
                }
            }
            free((char*)integer_list);
            return expr;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    asprintf(&expr, "%s", node->bool_expr.name);
                    return expr;
                }
                case AST_BOOL_NOT: {
                    asprintf(&expr, "not %s", node->bool_expr.name);
                    return expr;
                }
            }
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            asprintf(&expr, "%s = %llu", node->equality_expr.name, node->equality_expr.value.integer_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            asprintf(&expr, "%s = %.2f", node->equality_expr.name, node->equality_expr.value.float_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            asprintf(&expr, "%s = \"%s\"", node->equality_expr.name, node->equality_expr.value.string_value.string);
                            return expr;
                        }
                    }
                }
                case AST_EQUALITY_NE: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            asprintf(&expr, "%s <> %llu", node->equality_expr.name, node->equality_expr.value.integer_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            asprintf(&expr, "%s <> %.2f", node->equality_expr.name, node->equality_expr.value.float_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            asprintf(&expr, "%s <> \"%s\"", node->equality_expr.name, node->equality_expr.value.string_value.string);
                            return expr;
                        }
                    }
                }
            }
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s < %llu", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s < %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_LE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s <= %llu", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s <= %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s > %llu", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s > %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s >= %llu", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s >= %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                    }
                }
            }
        }
    }
}
