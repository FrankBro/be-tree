#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"
#include "utils.h"
#include "value.h"
#include "var.h"

int parse(const char *text, struct ast_node **node);

/*
void add_attr_domain_i(struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined);
void add_attr_domain_f(struct config* config, const char* attr, double min, double max, bool allow_undefined);
void add_attr_domain_b(struct config* config, const char* attr, bool min, bool max, bool allow_undefined);
void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined);
*/

void assign_expr(struct config* config, struct ast_node* node)
{
    assign_variable_id(config, node);
    assign_str_id(config, node);
}

int test_numeric_compare_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct numeric_compare_value i = { .value_type = AST_NUMERIC_COMPARE_VALUE_INTEGER, .integer_value = 12345 };

    {
        parse("i > 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GT, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer gt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i >= 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer ge");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i < 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LT, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer lt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i <= 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer le");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_numeric_compare_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "f", 0., 10., false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct numeric_compare_value f = { .value_type = AST_NUMERIC_COMPARE_VALUE_FLOAT, .float_value = 12345. };

    {
        parse("f > 12345.", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GT, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer gt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("f >= 12345.", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer ge");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("f < 12345.", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LT, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer lt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("f <= 12345.", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LE, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer le");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_numeric_compare_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);
    add_attr_domain_i(config, "i2", 0, 10, false);
    add_attr_domain_f(config, "f", 0., 10., false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct numeric_compare_value i = { .value_type = AST_NUMERIC_COMPARE_VALUE_INTEGER, .integer_value = 12345 };
    struct numeric_compare_value f = { .value_type = AST_NUMERIC_COMPARE_VALUE_FLOAT, .float_value = 12345. };

    {
        parse("i > 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GT, "i", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i > 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i2 > 12345", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i > 12346", &a);
        b = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong value");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_equality_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct equality_value i = { .value_type = AST_EQUALITY_VALUE_INTEGER, .integer_value = 12345 };

    {
        parse("i = 12345", &a);
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i <> 12345", &a);
        b = ast_equality_expr_create(AST_EQUALITY_NE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "integer ne");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_equality_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "f", 0., 10., false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct equality_value f = { .value_type = AST_EQUALITY_VALUE_FLOAT, .float_value = 12345. };

    {
        parse("f = 12345.", &a);
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "float eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("f <> 12345.", &a);
        b = ast_equality_expr_create(AST_EQUALITY_NE, "f", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "float ne");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_equality_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;

    {
        parse("s = \"12345\"", &a);
        struct string_value string = { .string = strdup("12345"), .str = -1ULL };
        struct equality_value s = { .value_type = AST_EQUALITY_VALUE_STRING, .string_value = string };
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "s", s);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "string eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("s <> \"12345\"", &a);
        struct string_value string = { .string = strdup("12345"), .str = -1ULL };
        struct equality_value s = { .value_type = AST_EQUALITY_VALUE_STRING, .string_value = string };
        b = ast_equality_expr_create(AST_EQUALITY_NE, "s", s);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "string ne");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_equality_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);
    add_attr_domain_i(config, "i2", 0, 10, false);
    add_attr_domain_f(config, "f", 0., 10., false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;
    struct equality_value i = { .value_type = AST_EQUALITY_VALUE_INTEGER, .integer_value = 12345 };
    struct equality_value f = { .value_type = AST_EQUALITY_VALUE_FLOAT, .float_value = 12345. };

    {
        parse("i = 12345", &a);
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "i", f);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i = 12345", &a);
        b = ast_equality_expr_create(AST_EQUALITY_NE, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i2 = 12345", &a);
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        parse("i = 12346", &a);
        b = ast_equality_expr_create(AST_EQUALITY_EQ, "i", i);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(!eq_expr(a, b), "wrong value");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_var_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;

    {
        struct attr_var lv = { .attr = strdup("i"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct integer_list_value rl = { .count = 0, .integers = NULL };
        add_integer_list_value(1, &rl);
        add_integer_list_value(2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST, .integer_list_value = rl };
        parse("i in (1,2)", &a);
        b = ast_set_expr_create(AST_SET_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "var in integer list");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct attr_var lv = { .attr = strdup("i"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct integer_list_value rl = { .count = 0, .integers = NULL };
        add_integer_list_value(1, &rl);
        add_integer_list_value(2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST, .integer_list_value = rl };
        parse("i not in (1,2)", &a);
        b = ast_set_expr_create(AST_SET_NOT_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "var not in integer list");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_var_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;

    {
        struct attr_var lv = { .attr = strdup("s"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct string_list_value rl = { .count = 0, .strings = NULL };
        struct string_value s1 = { .string = strdup("1"), .str = -1ULL };
        add_string_list_value(s1, &rl);
        struct string_value s2 = { .string = strdup("2"), .str = -1ULL };
        add_string_list_value(s2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_STRING_LIST, .string_list_value = rl };
        parse("s in (\"1\",\"2\")", &a);
        b = ast_set_expr_create(AST_SET_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "var in string list");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct attr_var lv = { .attr = strdup("s"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct string_list_value rl = { .count = 0, .strings = NULL };
        struct string_value s1 = { .string = strdup("1"), .str = -1ULL };
        add_string_list_value(s1, &rl);
        struct string_value s2 = { .string = strdup("2"), .str = -1ULL };
        add_string_list_value(s2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_STRING_LIST, .string_list_value = rl };
        parse("s not in (\"1\",\"2\")", &a);
        b = ast_set_expr_create(AST_SET_NOT_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "var not in string list");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_var_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    struct ast_node* a = NULL;
    struct ast_node* b = NULL;

    {
        struct attr_var lv = { .attr = strdup("i"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct integer_list_value rl = { .count = 0, .integers = NULL };
        add_integer_list_value(1, &rl);
        add_integer_list_value(2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST, .integer_list_value = rl };
        parse("i in (1,2)", &a);
        b = ast_set_expr_create(AST_SET_NOT_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct attr_var lv = { .attr = strdup("i"), .var = -1ULL };
        struct set_left_value l = { .value_type = AST_SET_LEFT_VALUE_VARIABLE, .variable_value = lv };
        struct integer_list_value rl = { .count = 0, .integers = NULL };
        add_integer_list_value(1, &rl);
        add_integer_list_value(2, &rl);
        struct set_right_value r = { .value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST, .integer_list_value = rl };
        parse("i not in (1,2)", &a);
        b = ast_set_expr_create(AST_SET_NOT_IN, l, r);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "var not in integer list");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int all_tests() 
{
    mu_run_test(test_numeric_compare_integer);
    mu_run_test(test_numeric_compare_float);
    mu_run_test(test_numeric_compare_wrong);
    mu_run_test(test_equality_integer);
    mu_run_test(test_equality_float);
    mu_run_test(test_equality_string);
    mu_run_test(test_equality_wrong);
    mu_run_test(test_set_var_integer);
    mu_run_test(test_set_var_string);
    //mu_run_test(test_set_var_wrong);
    /*
    mu_run_test(test_list);
    mu_run_test(test_special);
    */

    return 0;
}

RUN_TESTS()

