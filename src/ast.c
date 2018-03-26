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
    size_t length_copied = strlcpy((char*)node->binary_expr.name, name, VAR_NAME_MAX);
    if(length_copied > sizeof(node->binary_expr.name)) {
        fprintf(stderr, "Variable name is too big");
        exit(1);
    }
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
            break;
        case AST_TYPE_COMBI_EXPR:
            free_ast_node((struct ast_node*)node->combi_expr.lhs);
            free_ast_node((struct ast_node*)node->combi_expr.rhs);
            break;
    }
    free(node);
}

int get_variable(const char* name, const struct event* event)
{
    for(unsigned int i=0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(strcasecmp(name, pred->attr) == 0) {
            return pred->value;
        }
    }
    fprintf(stderr, "Variable %s not found in event", name);
    exit(1);
    return -1;
}

int match_node(const struct event* event, const struct ast_node *node)
{
    switch(node->type) {
        case AST_TYPE_BINARY_EXPR: {
            int variable = get_variable(node->binary_expr.name, event);
            switch(node->binary_expr.op) {
                case BINOP_LT: {
                    return variable < node->binary_expr.value;
                }
                case BINOP_LE: {
                    return variable <= node->binary_expr.value;
                }
                case BINOP_EQ: {
                    return variable == node->binary_expr.value;
                }
                case BINOP_NE: {
                    return variable != node->binary_expr.value;
                }
                case BINOP_GT: {
                    return variable > node->binary_expr.value;
                }
                case BINOP_GE: {
                    return variable >= node->binary_expr.value;
                }
            }
        }
        case AST_TYPE_COMBI_EXPR: {
            return match_node(event, node->combi_expr.lhs) && match_node(event, node->combi_expr.rhs);
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
                case BINOP_LT: {
                    bound->min = domain->minBound;
                    bound->max = fmax(bound->max, node->binary_expr.value - 1);
                    return;
                }
                case BINOP_LE: {
                    bound->min = domain->minBound;
                    bound->max = fmax(bound->max, node->binary_expr.value);
                    return;
                }
                case BINOP_EQ: {
                    bound->min = fmin(bound->min, node->binary_expr.value);
                    bound->max = fmax(bound->max, node->binary_expr.value);
                    return;
                }
                case BINOP_NE: {
                    bound->min = domain->minBound;
                    bound->max = domain->maxBound;
                    return;
                }
                case BINOP_GT: {
                    bound->min = fmin(bound->min, node->binary_expr.value + 1);
                    bound->max = domain->maxBound;
                    return;
                }
                case BINOP_GE: {
                    bound->min = fmin(bound->min, node->binary_expr.value);
                    bound->max = domain->maxBound;
                    return;
                }
            }
        }
    }
}