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
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_EQ, "EQ");
    free_ast_node(node);
    parse("a <> 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_NE, "NE");
    free_ast_node(node);
    parse("a < 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_LT, "LT");
    free_ast_node(node);
    parse("a <= 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_LE, "LE");
    free_ast_node(node);
    parse("a > 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_GT, "GT");
    free_ast_node(node);
    parse("a >= 0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && node->binary_expr.op == AST_BINOP_GE, "GE");
    free_ast_node(node);
    return 0;
}

int test_all_combi()
{
    struct ast_node* node = NULL;
    parse("a = 0 && b = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == AST_COMBI_AND, "AND");
    free_ast_node(node);
    parse("a = 0 || b = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == AST_COMBI_OR, "OR");
    free_ast_node(node);
    return 0;
}

int test_paren()
{
    struct ast_node* node = NULL;
    parse("(a = 0 && b = 0) || c = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == AST_COMBI_OR, "first");
    free_ast_node(node);
    parse("a = 0 && (b = 0 || c = 0)", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == AST_COMBI_AND, "second");
    free_ast_node(node);
    return 0;
}

// int test_precedence()
// {
//     struct ast_node* node = NULL;
//     parse("(a = 0 || b = 0 && c = 0 || d = 0", &node);
// }

int test_to_string()
{
    struct ast_node* node = NULL;

    {
        const char* expr = "a = 0 && b <> 0 && c > 0 || d >= 0 || e < 0 && f <= 0";
        parse(expr, &node);
        const char* to_string = ast_to_string(node);
        mu_assert(strcasecmp(expr, to_string) == 0, "");
        free_ast_node(node);
        free((char*)to_string);
    }

    return 0;
}

int all_tests() 
{
    mu_run_test(test_all_binop);
    mu_run_test(test_all_combi);
    mu_run_test(test_paren);
    // mu_run_test(test_precedence);
    mu_run_test(test_to_string);

    return 0;
}

RUN_TESTS()
