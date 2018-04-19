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

struct ast_node* ast_binary_expr_create(const enum ast_binop_e op, const char* name, struct value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_BINARY_EXPR;
    node->binary_expr.op = op;
    node->binary_expr.name = strdup(name);
    node->binary_expr.variable_id = -1;
    node->binary_expr.value = value;
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
        case AST_TYPE_BINARY_EXPR:
            free((char*)node->binary_expr.name);
            if(node->binary_expr.value.value_type == VALUE_S) {
                free((char*)node->binary_expr.value.svalue.string);
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

static const struct value false_value = { .value_type = VALUE_B, .bvalue = false };

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

struct value match_node(const struct event* event, const struct ast_node *node)
{
    switch(node->type) {
        case AST_TYPE_BOOL_EXPR: {
            struct value variable;
            bool found = get_variable(node->bool_expr.variable_id, event, &variable);
            if(!found) {
                return false_value;
            }
            struct value value = { .value_type = VALUE_B };
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    value.bvalue = variable.bvalue;
                    return value;
                }
                case AST_BOOL_NOT: {
                    value.bvalue = !variable.bvalue;
                    return value;
                }
            }
        }
        case AST_TYPE_LIST_EXPR: {
            struct value variable;
            bool found = get_variable(node->list_expr.variable_id, event, &variable);
            if(!found) {
                return false_value;
            }
            switch(node->list_expr.op) {
                case AST_LISTOP_NOTIN: {
                    if(variable.value_type != VALUE_I) {
                        invalid_expr("variable is not an integer in `not in` expression");
                    }
                    struct value value = { .value_type = VALUE_B, .bvalue = !integer_in_integer_list(variable.ivalue, node->list_expr.list) };
                    return value;
                }
                case AST_LISTOP_IN: {
                    if(variable.value_type != VALUE_I) {
                        invalid_expr("variable is not an integer in `in` expression");
                    }
                    struct value value = { .value_type = VALUE_B, .bvalue = integer_in_integer_list(variable.ivalue, node->list_expr.list) };
                    return value;
                }
            }
        }
        case AST_TYPE_BINARY_EXPR: {
            struct value variable;
            bool found = get_variable(node->binary_expr.variable_id, event, &variable);
            if(!found) {
                return false_value;
            }
            if(variable.value_type != node->binary_expr.value.value_type) {
                invalid_expr("%s value types do not match");
            }
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue < node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.fvalue < node->binary_expr.value.fvalue };
                            return value;
                        }
                        case VALUE_B: {
                            invalid_expr("Using < on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using < on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using < on a integer list value");
                        }
                    }
                }
                case AST_BINOP_LE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue <= node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.fvalue <= node->binary_expr.value.fvalue };
                            return value;
                        }
                        case VALUE_B: {
                            invalid_expr("Using <= on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using <= on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <= on a integer list value");
                        }
                    }
                }
                case AST_BINOP_EQ: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue == node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = feq(variable.fvalue, node->binary_expr.value.fvalue) };
                            return value;
                        }
                        case VALUE_B: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.bvalue == node->binary_expr.value.bvalue };
                            return value;
                        }
                        case VALUE_S: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.svalue.str == node->binary_expr.value.svalue.str };
                            return value;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using = on a integer list value");
                        }
                    }
                }
                case AST_BINOP_NE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue != node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = fne(variable.fvalue, node->binary_expr.value.fvalue) };
                            return value;
                        }
                        case VALUE_B: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.bvalue != node->binary_expr.value.bvalue };
                            return value;
                        }
                        case VALUE_S: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.svalue.str != node->binary_expr.value.svalue.str };
                            return value;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <> on a integer list value");
                        }
                    }
                }
                case AST_BINOP_GT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue > node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.fvalue > node->binary_expr.value.fvalue };
                            return value;
                        }
                        case VALUE_B: {
                            invalid_expr("Using > on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using > on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using > on a integer list value");
                        }
                    }
                }
                case AST_BINOP_GE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.ivalue >= node->binary_expr.value.ivalue };
                            return value;
                        }
                        case VALUE_F: {
                            struct value value = { .value_type = VALUE_B, .bvalue = variable.fvalue >= node->binary_expr.value.fvalue };
                            return value;
                        }
                        case VALUE_B: {
                            invalid_expr("Using >= on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using >= on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using >= on a integer list value");
                        }
                    }
                }
            }
        }
        case AST_TYPE_COMBI_EXPR: {
            struct value lhs = match_node(event, node->combi_expr.lhs);
            struct value rhs = match_node(event, node->combi_expr.rhs);
            if(lhs.value_type != VALUE_B || rhs.value_type != VALUE_B) {
                invalid_expr("Using && or || on a non-boolean value");
            }
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    struct value value = { .value_type = VALUE_B, .bvalue = lhs.bvalue && rhs.bvalue };
                    return value;
                }
                case AST_COMBI_OR: {
                    struct value value = { .value_type = VALUE_B, .bvalue = lhs.bvalue || rhs.bvalue };
                    return value;
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
        case AST_TYPE_BINARY_EXPR: {
            if(domain->bound.value_type != bound->value_type || domain->bound.value_type != node->binary_expr.value.value_type) {
                invalid_expr("Domain, bound or expr type mismatch");
            }
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = domain->bound.imin;
                            bound->imax = max(bound->imax, node->binary_expr.value.ivalue - 1);
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->binary_expr.value.fvalue - __DBL_EPSILON__);
                            return;
                        }
                        case VALUE_B: {
                            invalid_expr("Using < on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using < on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using < on a integer list value");
                        }
                    }
                }
                case AST_BINOP_LE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = domain->bound.imin;
                            bound->imax = max(bound->imax, node->binary_expr.value.ivalue);
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->binary_expr.value.fvalue);
                            return;
                        }
                        case VALUE_B: {
                            invalid_expr("Using <= on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using <= on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <= on a integer list value");
                        }
                    }
                }
                case AST_BINOP_EQ: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = min(bound->imin, node->binary_expr.value.ivalue);
                            bound->imax = max(bound->imax, node->binary_expr.value.ivalue);
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = fmin(bound->fmin, node->binary_expr.value.fvalue);
                            bound->fmax = fmax(bound->fmax, node->binary_expr.value.fvalue);
                            return;
                        }
                        case VALUE_B: {
                            bound->bmin = min(bound->bmin, node->binary_expr.value.bvalue);
                            bound->bmax = max(bound->bmin, node->binary_expr.value.bvalue);
                            return;
                        }
                        case VALUE_S: {
                            invalid_expr("Trying to get the bound of a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using = on a integer list value");
                        }
                    }
                }
                case AST_BINOP_NE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = domain->bound.imin;
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        case VALUE_B: {
                            bound->bmin = domain->bound.bmin;
                            bound->bmax = domain->bound.bmax;
                        }
                        case VALUE_S: {
                            invalid_expr("Trying to get the bound of a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <> on a integer list value");
                        }
                    }
                }
                case AST_BINOP_GT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = min(bound->imin, node->binary_expr.value.ivalue + 1);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = fmin(bound->fmin, node->binary_expr.value.fvalue + __DBL_EPSILON__);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        case VALUE_B: {
                            invalid_expr("Using > on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using > on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using > on a integer list value");
                        }
                    }
                }
                case AST_BINOP_GE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            bound->imin = min(bound->imin, node->binary_expr.value.ivalue);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case VALUE_F: {
                            bound->fmin = fmin(bound->fmin, node->binary_expr.value.fvalue);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        case VALUE_B: {
                            invalid_expr("Using >= on a boolean value");
                        }
                        case VALUE_S: {
                            invalid_expr("Using >= on a string value");
                        }
                        case VALUE_IL: {
                            invalid_expr("Using >= on a integer list value");
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
        case(AST_TYPE_BINARY_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->binary_expr.name);
            node->binary_expr.variable_id = variable_id;
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
        case(AST_TYPE_BINARY_EXPR): {
            if(node->binary_expr.value.value_type == VALUE_S) {
                betree_str_t str_id = get_id_for_string(config, node->binary_expr.value.svalue.string);
                node->binary_expr.value.svalue.str = str_id;
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
        case(AST_TYPE_BINARY_EXPR): {
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s < %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s < %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            invalid_expr("Using < on a boolean value");
                            return NULL;
                        }
                        case VALUE_S: {
                            invalid_expr("Using < on a string value");
                            return NULL;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using < on a integer list value");
                            return NULL;
                        }
                    }
                }
                case AST_BINOP_LE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s <= %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s <= %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            invalid_expr("Using <= on a boolean value");
                            return NULL;
                        }
                        case VALUE_S: {
                            invalid_expr("Using <= on a string value");
                            return NULL;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <= on a integer list value");
                            return NULL;
                        }
                    }
                }
                case AST_BINOP_EQ: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s = %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s = %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            asprintf(&expr, "%s = %s", node->binary_expr.name, node->binary_expr.value.bvalue ? "true" : "false");
                            return expr;
                        }
                        case VALUE_S: {
                            asprintf(&expr, "%s = \"%s\"", node->binary_expr.name, node->binary_expr.value.svalue.string);
                            return expr;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using = on a integer list value");
                            return NULL;
                        }
                    }
                }
                case AST_BINOP_NE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s <> %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s <> %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            asprintf(&expr, "%s <> %s", node->binary_expr.name, node->binary_expr.value.bvalue ? "true" : "false");
                            return expr;
                        }
                        case VALUE_S: {
                            asprintf(&expr, "%s <> \"%s\"", node->binary_expr.name, node->binary_expr.value.svalue.string);
                            return expr;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using <> on a integer list value");
                            return NULL;
                        }
                    }
                }
                case AST_BINOP_GT: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s > %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s > %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            invalid_expr("Using > on a boolean value");
                            return NULL;
                        }
                        case VALUE_S: {
                            invalid_expr("Using > on a string value");
                            return NULL;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using > on a integer list value");
                            return NULL;
                        }
                    }
                }
                case AST_BINOP_GE: {
                    switch(node->binary_expr.value.value_type) {
                        case VALUE_I: {
                            asprintf(&expr, "%s >= %llu", node->binary_expr.name, node->binary_expr.value.ivalue);
                            return expr;
                        }
                        case VALUE_F: {
                            asprintf(&expr, "%s >= %.2f", node->binary_expr.name, node->binary_expr.value.fvalue);
                            return expr;
                        }
                        case VALUE_B: {
                            invalid_expr("Using >= on a boolean value");
                            return NULL;
                        }
                        case VALUE_S: {
                            invalid_expr("Using >= on a string value");
                            return NULL;
                        }
                        case VALUE_IL: {
                            invalid_expr("Using >= on a integer list value");
                            return NULL;
                        }
                    }
                }
            }
        }
    }
}
