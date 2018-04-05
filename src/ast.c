#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"

struct ast_node* ast_node_create()
{
    struct ast_node* node = malloc(sizeof(struct ast_node));
    return node;
}

struct ast_node* ast_binary_expr_create(const enum ast_binop_e op, const char* name, int value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_BINARY_EXPR;
    node->binary_expr.op = op;
    node->binary_expr.name = strdup(name);
    node->binary_expr.variable_id = -1;
    node->binary_expr.value = value;
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

void free_ast_node(struct ast_node* node)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_BINARY_EXPR:
            free((char*)node->binary_expr.name);
            break;
        case AST_TYPE_COMBI_EXPR:
            free_ast_node((struct ast_node*)node->combi_expr.lhs);
            free_ast_node((struct ast_node*)node->combi_expr.rhs);
            break;
    }
    free(node);
}

bool get_variable(unsigned int variable_id, const struct event* event, int* value)
{
    for(unsigned int i=0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(variable_id == pred->variable_id) {
            *value = pred->value;
            return true;
        }
    }
    return false;
}

int match_node(const struct event* event, const struct ast_node *node)
{
    switch(node->type) {
        case AST_TYPE_BINARY_EXPR: {
            int variable;
            bool found = get_variable(node->binary_expr.variable_id, event, &variable);
            if(!found) {
                return 0;
            }
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    return variable < node->binary_expr.value;
                }
                case AST_BINOP_LE: {
                    return variable <= node->binary_expr.value;
                }
                case AST_BINOP_EQ: {
                    return variable == node->binary_expr.value;
                }
                case AST_BINOP_NE: {
                    return variable != node->binary_expr.value;
                }
                case AST_BINOP_GT: {
                    return variable > node->binary_expr.value;
                }
                case AST_BINOP_GE: {
                    return variable >= node->binary_expr.value;
                }
            }
        }
        case AST_TYPE_COMBI_EXPR: {
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    return match_node(event, node->combi_expr.lhs) && match_node(event, node->combi_expr.rhs);
                }
                case AST_COMBI_OR: {
                    return match_node(event, node->combi_expr.lhs) || match_node(event, node->combi_expr.rhs);
                }
            }
        }
    }
}

void get_variable_bound(const struct attr_domain* domain, const struct ast_node* node, struct variable_bound* bound)
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
        case AST_TYPE_BINARY_EXPR: {
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    bound->min = domain->min_bound;
                    bound->max = fmax(bound->max, node->binary_expr.value - 1);
                    return;
                }
                case AST_BINOP_LE: {
                    bound->min = domain->min_bound;
                    bound->max = fmax(bound->max, node->binary_expr.value);
                    return;
                }
                case AST_BINOP_EQ: {
                    bound->min = fmin(bound->min, node->binary_expr.value);
                    bound->max = fmax(bound->max, node->binary_expr.value);
                    return;
                }
                case AST_BINOP_NE: {
                    bound->min = domain->min_bound;
                    bound->max = domain->max_bound;
                    return;
                }
                case AST_BINOP_GT: {
                    bound->min = fmin(bound->min, node->binary_expr.value + 1);
                    bound->max = domain->max_bound;
                    return;
                }
                case AST_BINOP_GE: {
                    bound->min = fmin(bound->min, node->binary_expr.value);
                    bound->max = domain->max_bound;
                    return;
                }
            }
        }
    }
}

void assign_variable_id(struct config* config, struct ast_node* node) 
{
    switch(node->type) {
        case(AST_TYPE_BINARY_EXPR): {
            unsigned int variable_id = get_id_for_attr(config, node->binary_expr.name);
            node->binary_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_COMBI_EXPR): {
            assign_variable_id(config, (struct ast_node*)node->combi_expr.lhs);
            assign_variable_id(config, (struct ast_node*)node->combi_expr.rhs);
            return;
        }
    }
}

const char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case(AST_TYPE_COMBI_EXPR): {
            char* a = ast_to_string(node->combi_expr.lhs);
            char* b = ast_to_string(node->combi_expr.rhs);
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
            free(a);
            free(b);
            return expr;

        }
        case(AST_TYPE_BINARY_EXPR): {
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    asprintf(&expr, "%s < %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
                case AST_BINOP_LE: {
                    asprintf(&expr, "%s <= %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
                case AST_BINOP_EQ: {
                    asprintf(&expr, "%s = %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
                case AST_BINOP_NE: {
                    asprintf(&expr, "%s <> %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
                case AST_BINOP_GT: {
                    asprintf(&expr, "%s > %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
                case AST_BINOP_GE: {
                    asprintf(&expr, "%s >= %d", node->binary_expr.name, node->binary_expr.value);
                    return expr;
                }
            }
        }
    }
}
