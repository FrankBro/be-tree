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



struct ast_node* parse_and_assign(const char* expr, struct config* config)
{
    struct ast_node* node = NULL;
    parse(expr, &node);
    assign_expr(config, node);
    return node;
}

int test_numeric_compare_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("i > 12345", config);
        struct ast_node* b = parse_and_assign("i > 12345", config);
        mu_assert(eq_expr(a, b), "integer gt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i >= 12345", config);
        struct ast_node* b = parse_and_assign("i >= 12345", config);
        mu_assert(eq_expr(a, b), "integer ge");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i < 12345", config);
        struct ast_node* b = parse_and_assign("i < 12345", config);
        mu_assert(eq_expr(a, b), "integer lt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i <= 12345", config);
        struct ast_node* b = parse_and_assign("i <= 12345", config);
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

    {
        struct ast_node* a = parse_and_assign("f > 12345.", config);
        struct ast_node* b = parse_and_assign("f > 12345.", config);
        mu_assert(eq_expr(a, b), "integer gt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("f >= 12345.", config);
        struct ast_node* b = parse_and_assign("f >= 12345.", config);
        mu_assert(eq_expr(a, b), "integer ge");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("f < 12345.", config);
        struct ast_node* b = parse_and_assign("f < 12345.", config);
        mu_assert(eq_expr(a, b), "integer lt");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("f <= 12345.", config);
        struct ast_node* b = parse_and_assign("f <= 12345.", config);
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

    {
        struct ast_node* a = parse_and_assign("i > 12345", config);
        struct ast_node* b = parse_and_assign("i > 12345.", config);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i > 12345", config);
        struct ast_node* b = parse_and_assign("i >= 12345", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i2 > 12345", config);
        struct ast_node* b = parse_and_assign("i > 12345", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i > 12346", config);
        struct ast_node* b = parse_and_assign("i > 12345", config);
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

    {
        struct ast_node* a = parse_and_assign("i = 12345", config);
        struct ast_node* b = parse_and_assign("i = 12345", config);
        mu_assert(eq_expr(a, b), "integer eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i <> 12345", config);
        struct ast_node* b = parse_and_assign("i <> 12345", config);
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

    {
        struct ast_node* a = parse_and_assign("f = 12345.", config);
        struct ast_node* b = parse_and_assign("f = 12345.", config);
        mu_assert(eq_expr(a, b), "float eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("f <> 12345.", config);
        struct ast_node* b = parse_and_assign("f <> 12345.", config);
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

    {
        struct ast_node* a = parse_and_assign("s = \"12345\"", config);
        struct ast_node* b = parse_and_assign("s = \"12345\"", config);
        assign_expr(config, a);
        assign_expr(config, b);
        mu_assert(eq_expr(a, b), "string eq");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("s <> \"12345\"", config);
        struct ast_node* b = parse_and_assign("s <> \"12345\"", config);
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

    {
        struct ast_node* a = parse_and_assign("i = 12345", config);
        struct ast_node* b = parse_and_assign("i = 12345.", config);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i = 12345", config);
        struct ast_node* b = parse_and_assign("i <> 12345", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i2 = 12345", config);
        struct ast_node* b = parse_and_assign("i = 12345", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i = 12346", config);
        struct ast_node* b = parse_and_assign("i = 12345", config);
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

    {
        struct ast_node* a = parse_and_assign("i in (1,2)", config);
        struct ast_node* b = parse_and_assign("i in (1,2)", config);
        mu_assert(eq_expr(a, b), "var in integer list");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i not in (1,2)", config);
        struct ast_node* b = parse_and_assign("i not in (1,2)", config);
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

    {
        struct ast_node* a = parse_and_assign("s in (\"1\",\"2\")", config);
        struct ast_node* b = parse_and_assign("s in (\"1\",\"2\")", config);
        mu_assert(eq_expr(a, b), "var in string list");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("s not in (\"1\",\"2\")", config);
        struct ast_node* b = parse_and_assign("s not in (\"1\",\"2\")", config);
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
    add_attr_domain_i(config, "i2", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("i in (1, 2)", config);
        struct ast_node* b = parse_and_assign("i in (\"1\", \"2\")", config);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i in (1, 2)", config);
        struct ast_node* b = parse_and_assign("i not in (1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i2 in (1, 2)", config);
        struct ast_node* b = parse_and_assign("i in (1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i in (1, 2)", config);
        struct ast_node* b = parse_and_assign("i in (1, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong value");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_list_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "il", false);

    {
        struct ast_node* a = parse_and_assign("1 in il", config);
        struct ast_node* b = parse_and_assign("1 in il", config);
        mu_assert(eq_expr(a, b), "integer in var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("1 not in il", config);
        struct ast_node* b = parse_and_assign("1 not in il", config);
        mu_assert(eq_expr(a, b), "integer not in var");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_list_string()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "sl", false);

    {
        struct ast_node* a = parse_and_assign("\"1\" in sl", config);
        struct ast_node* b = parse_and_assign("\"1\" in sl", config);
        mu_assert(eq_expr(a, b), "string in var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("\"1\" not in sl", config);
        struct ast_node* b = parse_and_assign("\"1\" not in sl", config);
        mu_assert(eq_expr(a, b), "string not in var");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_set_list_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "il", false);
    add_attr_domain_il(config, "il2", false);

    {
        struct ast_node* a = parse_and_assign("1 in il", config);
        struct ast_node* b = parse_and_assign("\"1\" in il", config);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("1 in il", config);
        struct ast_node* b = parse_and_assign("1 not in il", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("1 in il", config);
        struct ast_node* b = parse_and_assign("1 in il2", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("1 in il", config);
        struct ast_node* b = parse_and_assign("2 in il", config);
        mu_assert(!eq_expr(a, b), "wrong value");
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
    mu_run_test(test_set_var_wrong);
    mu_run_test(test_set_list_integer);
    mu_run_test(test_set_list_string);
    mu_run_test(test_set_list_wrong);
    /*
    mu_run_test(test_list);
    mu_run_test(test_special);
    */

    return 0;
}

RUN_TESTS()

