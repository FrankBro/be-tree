#ifndef AST_H__
#define AST_H__

#include "betree.h"

enum ast_binop_e {
    AST_BINOP_LT,
    AST_BINOP_LE,
    AST_BINOP_EQ,
    AST_BINOP_NE,
    AST_BINOP_GT,
    AST_BINOP_GE,
};

struct ast_binary_expr {
    enum ast_binop_e op;
    unsigned int variable_id;
    const char *name;
    int value;
};

enum ast_combi_e {
    AST_COMBI_OR,
    AST_COMBI_AND,
};

struct ast_node;

struct ast_combi_expr {
    enum ast_combi_e op;
    const struct ast_node* lhs;
    const struct ast_node* rhs;
};

enum ast_node_type_e {
    AST_TYPE_BINARY_EXPR,
    AST_TYPE_COMBI_EXPR,
};

struct ast_node {
    enum ast_node_type_e type;
    union {
        struct ast_binary_expr binary_expr;
        struct ast_combi_expr combi_expr;
    };
};

struct ast_node* ast_binary_expr_create(const enum ast_binop_e op, const char* name, int value);
struct ast_node* ast_combi_expr_create(const enum ast_combi_e op, const struct ast_node* lhs, const struct ast_node* rhs);
void free_ast_node(struct ast_node* node);

int match_node(const struct event* event, const struct ast_node *node);

struct variable_bound {
    int min;
    int max;
};

void get_variable_bound(const struct attr_domain* domain, const struct ast_node* node, struct variable_bound* bound);

void assign_variable_id(struct config* config, struct ast_node* node);

#endif
