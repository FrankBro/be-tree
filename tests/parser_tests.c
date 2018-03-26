#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"

int parse(const char *text, struct ast_node **node);

int test_all_binop()
{
    struct ast_node* node = NULL;
    parse("a = 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_EQ, "EQ");
    parse("a <> 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_NE, "NE");
    parse("a < 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_LT, "LT");
    parse("a <= 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_LE, "LE");
    parse("a > 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_GT, "GT");
    parse("a >= 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == BINOP_GE, "GE");
    return 0;
}

int test_all_combi()
{
    struct ast_node* node = NULL;
    parse("a = 0 && b = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == COMBI_AND, "AND");
    parse("a = 0 || b = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == COMBI_OR, "OR");
    return 0;
}

int test_paren()
{
    struct ast_node* node = NULL;
    parse("(a = 0 && b = 0) || c = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == COMBI_OR, "first");
    parse("a = 0 && (b = 0 || c = 0)", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == COMBI_AND, "second");
    return 0;
}

int all_tests() 
{
    mu_run_test(test_all_binop);
    mu_run_test(test_all_combi);
    mu_run_test(test_paren);

    return 0;
}

RUN_TESTS()
