#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"
#include "utils.h"

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
    parse("a = 0 and b = 0", &node);
    mu_assert(node->type == AST_TYPE_COMBI_EXPR && node->combi_expr.op == AST_COMBI_AND, "AND");
    free_ast_node(node);
    parse("a = 0 or b = 0", &node);
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

int test_float()
{
    struct ast_node* node = NULL;
    parse("a = 0.", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && 
        node->binary_expr.value.value_type == VALUE_F && 
        feq(node->binary_expr.value.fvalue, 0.)
    , "no decimal");
    free_ast_node(node);
    parse("a = 0.0", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR && 
        node->binary_expr.value.value_type == VALUE_F && 
        feq(node->binary_expr.value.fvalue, 0.)
    , "with decimal");
    free_ast_node(node);
    return 0;
}

int test_bool()
{
    struct ast_node* node = NULL;
    parse("a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR &&
        node->bool_expr.op == AST_BOOL_NONE
    , "none");
    free_ast_node(node);
    parse("not a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR &&
        node->bool_expr.op == AST_BOOL_NOT
    , "not");
    free_ast_node(node);
    return 0;
}

int test_string()
{
    struct ast_node* node = NULL;
    parse("a = \"a\"", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR &&
        node->binary_expr.op == AST_BINOP_EQ &&
        node->binary_expr.value.value_type == VALUE_S
    , "eq");
    free_ast_node(node);
    parse("a <> \"a\"", &node);
    mu_assert(node->type == AST_TYPE_BINARY_EXPR &&
        node->binary_expr.op == AST_BINOP_NE &&
        node->binary_expr.value.value_type == VALUE_S
    , "ne");
    free_ast_node(node);
    return 0;
}

int test_integer_list()
{
    struct ast_node* node = NULL;
    parse("a in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LISTOP_IN
    , "in");
    free_ast_node(node);
    parse("a not in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LISTOP_NOTIN
    , "in");
    free_ast_node(node);
    parse("a in (1)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LISTOP_IN
    , "single");
    free_ast_node(node);
    return 0;
}

int all_tests() 
{
    mu_run_test(test_all_binop);
    mu_run_test(test_all_combi);
    mu_run_test(test_paren);
    // mu_run_test(test_precedence);
    mu_run_test(test_to_string);
    mu_run_test(test_float);
    mu_run_test(test_bool);
    mu_run_test(test_string);
    mu_run_test(test_integer_list);

    return 0;
}

RUN_TESTS()
