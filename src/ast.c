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

struct ast_node* ast_set_expr_create(const enum ast_set_e op, struct set_left_value left_value, struct set_right_value right_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_SET_EXPR;
    node->set_expr.op = op;
    node->set_expr.left_value = left_value;
    node->set_expr.right_value = right_value;
    return node;
}

struct ast_node* ast_list_expr_create(const enum ast_list_e op, const char* name, struct list_value list_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_LIST_EXPR;
    node->list_expr.op = op;
    node->list_expr.name = strdup(name);
    node->list_expr.variable_id = -1;
    node->list_expr.value = list_value;
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
        case AST_TYPE_SET_EXPR:
            switch(node->set_expr.left_value.value_type) {
                case AST_SET_LEFT_VALUE_INTEGER: {
                    break;
                }
                case AST_SET_LEFT_VALUE_STRING: {
                    free((char*)node->set_expr.left_value.string_value.string);
                    break;
                }
                case AST_SET_LEFT_VALUE_VARIABLE: {
                    free((char*)node->set_expr.left_value.variable_value.name);
                    break;
                }
            }
            switch(node->set_expr.right_value.value_type) {
                case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                    free(node->set_expr.right_value.integer_list_value.integers);
                    break;
                }
                case AST_SET_RIGHT_VALUE_STRING_LIST: {
                    for(size_t i = 0; i < node->set_expr.right_value.string_list_value.count; i++) {
                        free((char*)node->set_expr.right_value.string_list_value.strings[i].string);
                    }
                    free(node->set_expr.right_value.string_list_value.strings);
                    break;
                }
                case AST_SET_RIGHT_VALUE_VARIABLE: {
                    free((char*)node->set_expr.right_value.variable_value.name);
                    break;
                }
            }
            break;
        case AST_TYPE_LIST_EXPR:
            switch(node->list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: {
                    free(node->list_expr.value.integer_list_value.integers);
                    break;
                }
                case AST_LIST_VALUE_STRING_LIST: {
                    for(size_t i = 0; i < node->list_expr.value.string_list_value.count; i++) {
                        free((char*)node->list_expr.value.string_list_value.strings[i].string);
                    }
                    free(node->list_expr.value.string_list_value.strings);
                    break;
                }
            }
            free((char*)node->list_expr.name);
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

bool integer_in_integer_list(int64_t integer, struct integer_list_value list)
{
    for(size_t i = 0; i < list.count; i++) {
        if(list.integers[i] == integer) {
            return true;
        }
    }
    return false;
}

bool string_in_string_list(struct string_value string, struct string_list_value list)
{
    for(size_t i = 0; i < list.count; i++) {
        if(list.strings[i].str == string.str) {
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

bool list_value_matches(enum ast_list_value_e a, enum value_e b) {
    return
        (a == AST_LIST_VALUE_INTEGER_LIST && b == VALUE_IL) ||
        (a == AST_LIST_VALUE_STRING_LIST && b == VALUE_SL);
}

bool match_node(const struct event* event, const struct ast_node *node)
{
    // TODO allow undefined handling?
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
            if(!list_value_matches(node->list_expr.value.value_type, variable.value_type)) {
                invalid_expr("list value types do not match");
            }
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF: 
                case AST_LIST_NONE_OF: {
                    bool result = false;
                    switch(node->list_expr.value.value_type) {
                        case AST_LIST_VALUE_INTEGER_LIST: {
                            for(size_t i = 0; i < variable.ilvalue.count; i++) {
                                int64_t left = variable.ilvalue.integers[i];
                                for(size_t j = 0; j < node->list_expr.value.integer_list_value.count; j++) {
                                    int64_t right = node->list_expr.value.integer_list_value.integers[j];
                                    if(left == right) {
                                        result = true;
                                        break;
                                    }
                                }
                                if(result == true) {
                                    break;
                                }
                            }
                            break;
                        }
                        case AST_LIST_VALUE_STRING_LIST: {
                            for(size_t i = 0; i < variable.slvalue.count; i++) {
                                betree_str_t left = variable.slvalue.strings[i].str;
                                for(size_t j = 0; j < node->list_expr.value.string_list_value.count; j++) {
                                    betree_str_t right = node->list_expr.value.string_list_value.strings[j].str;
                                    if(left == right) {
                                        result = true;
                                        break;
                                    }
                                }
                                if(result == true) {
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    switch(node->list_expr.op) {
                        case AST_LIST_ONE_OF: 
                            return result;
                        case AST_LIST_NONE_OF: 
                            return !result;
                        case AST_LIST_ALL_OF:
                            invalid_expr("Should never happen");
                    }
                }
                case AST_LIST_ALL_OF: {
                    size_t count = 0, target_count = 0;
                    switch(node->list_expr.value.value_type) {
                        case AST_LIST_VALUE_INTEGER_LIST: {
                            target_count = node->list_expr.value.integer_list_value.count;
                            for(size_t i = 0; i < target_count; i++) {
                                int64_t right = node->list_expr.value.integer_list_value.integers[i];
                                for(size_t j = 0; j < variable.ilvalue.count; j++) {
                                    int64_t left = variable.ilvalue.integers[j];
                                    if(left == right) {
                                        count++;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case AST_LIST_VALUE_STRING_LIST: {
                            target_count = node->list_expr.value.string_list_value.count;
                            for(size_t i = 0; i < target_count; i++) {
                                betree_str_t right = node->list_expr.value.string_list_value.strings[i].str;
                                for(size_t j = 0; j < variable.slvalue.count; j++) {
                                    betree_str_t left = variable.slvalue.strings[j].str;
                                    if(left == right) {
                                        count++;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                    return count == target_count;
                }
            }
        }
        case AST_TYPE_SET_EXPR: {
            struct set_left_value left = node->set_expr.left_value;
            struct set_right_value right = node->set_expr.right_value;
            struct value variable;
            bool is_in;
            if(left.value_type == AST_SET_LEFT_VALUE_INTEGER && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                bool found = get_variable(right.variable_value.variable_id, event, &variable);
                if(!found) {
                    return false;
                }
                if(variable.value_type != VALUE_IL) {
                    invalid_expr("invalid variable in set expression, should be integer list");
                }
                is_in = integer_in_integer_list(left.integer_value, variable.ilvalue);
            }
            else if(left.value_type == AST_SET_LEFT_VALUE_STRING && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                bool found = get_variable(right.variable_value.variable_id, event, &variable);
                if(!found) {
                    return false;
                }
                if(variable.value_type != VALUE_SL) {
                    invalid_expr("invalid variable in set expression, should be string list");
                }
                is_in = string_in_string_list(left.string_value, variable.slvalue);
            }
            else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
                bool found = get_variable(left.variable_value.variable_id, event, &variable);
                if(!found) {
                    return false;
                }
                if(variable.value_type != VALUE_I) {
                    invalid_expr("invalid variable in set expression, should be integer");
                }
                is_in = integer_in_integer_list(variable.ivalue, right.integer_list_value);
            }
            else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                bool found = get_variable(left.variable_value.variable_id, event, &variable);
                if(!found) {
                    return false;
                }
                if(variable.value_type != VALUE_S) {
                    invalid_expr("invalid variable in set expression, should be string");
                }
                is_in = string_in_string_list(variable.svalue, right.string_list_value);
            }
            else {
                invalid_expr("invalid set expression");
            }
            switch(node->set_expr.op) {
                case AST_SET_NOT_IN: {
                    return !is_in;
                }
                case AST_SET_IN: {
                    return is_in;
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
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    if(lhs == false) {
                        return false;
                    }
                    bool rhs = match_node(event, node->combi_expr.rhs);
                    return lhs && rhs;
                }
                case AST_COMBI_OR: {
                    bool rhs = match_node(event, node->combi_expr.rhs);
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
            invalid_expr("Trying to get the bound of an list expression");
        }
        case AST_TYPE_SET_EXPR: {
            invalid_expr("Trying to get the bound of an set expression");
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
        case(AST_TYPE_SET_EXPR): {
            switch(node->set_expr.left_value.value_type) {
                case AST_SET_LEFT_VALUE_INTEGER: {
                    return;
                }
                case AST_SET_LEFT_VALUE_STRING: {
                    return;
                }
                case AST_SET_LEFT_VALUE_VARIABLE: {
                    betree_var_t variable_id = get_id_for_attr(config, node->set_expr.left_value.variable_value.name);
                    node->set_expr.left_value.variable_value.variable_id = variable_id;
                    return;
                }
            }
            switch(node->set_expr.right_value.value_type) {
                case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                    return;
                }
                case AST_SET_RIGHT_VALUE_STRING_LIST: {
                    return;
                }
                case AST_SET_RIGHT_VALUE_VARIABLE: {
                    betree_var_t variable_id = get_id_for_attr(config, node->set_expr.right_value.variable_value.name);
                    node->set_expr.right_value.variable_value.variable_id = variable_id;
                    return;
                }
            }
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
            if(node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->list_expr.value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(config, node->list_expr.value.string_list_value.strings[i].string);
                    node->list_expr.value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config, node->set_expr.left_value.string_value.string);
                node->set_expr.left_value.string_value.str = str_id;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->set_expr.right_value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(config, node->set_expr.right_value.string_list_value.strings[i].string);
                    node->set_expr.right_value.string_list_value.strings[i].str = str_id;
                }
            }
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
        case(AST_TYPE_SET_EXPR): {
            switch(node->set_expr.left_value.value_type) {
                case AST_SET_LEFT_VALUE_INTEGER: {
                    asprintf(&expr, "%llu ", node->set_expr.left_value.integer_value);
                    break;
                }
                case AST_SET_LEFT_VALUE_STRING: {
                    asprintf(&expr, "\"%s\" ", node->set_expr.left_value.string_value.string);
                    break;
                }
                case AST_SET_LEFT_VALUE_VARIABLE: {
                    asprintf(&expr, "%s ", node->set_expr.left_value.variable_value.name);
                    break;
                }
            }
            switch(node->set_expr.op) {
                case AST_SET_NOT_IN: {
                    asprintf(&expr, "not in ");
                    break;
                }
                case AST_SET_IN: {
                    asprintf(&expr, "in ");
                    break;
                }
            }
            switch(node->set_expr.right_value.value_type) {
                case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                    const char* list = integer_list_value_to_string(node->set_expr.right_value.integer_list_value);
                    asprintf(&expr, "(%s) ", list);
                    free((char*)list);
                    break;
                }
                case AST_SET_RIGHT_VALUE_STRING_LIST: {
                    const char* list = string_list_value_to_string(node->set_expr.right_value.string_list_value);
                    asprintf(&expr, "(%s) ", list);
                    free((char*)list);
                    break;
                }
                case AST_SET_RIGHT_VALUE_VARIABLE: {
                    asprintf(&expr, "%s ", node->set_expr.right_value.variable_value.name);
                    break;
                }
            }
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            char* list;
            switch(node->list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: {
                    const char* inner = integer_list_value_to_string(node->list_expr.value.integer_list_value);
                    asprintf(&list, "(%s) ", inner);
                    free((char*)inner);
                    break;
                }
                case AST_LIST_VALUE_STRING_LIST: {
                    const char* inner = string_list_value_to_string(node->list_expr.value.string_list_value);
                    asprintf(&list, "(%s) ", inner);
                    free((char*)inner);
                    break;
                }
            }
            char* op;
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF: {
                    asprintf(&op, "one of ");
                    break;
                }
                case AST_LIST_NONE_OF: {
                    asprintf(&op, "none of ");
                    break;
                }
                case AST_LIST_ALL_OF: {
                    asprintf(&op, "all of ");
                    break;
                }
            }
            asprintf(&expr, "%s %s %s", node->list_expr.name, op, list);
            free(list);
            free(op);
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
