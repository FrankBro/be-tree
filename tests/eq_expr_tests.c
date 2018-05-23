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

int test_list_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("i one of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i one of (1, 2)", config);
        mu_assert(eq_expr(a, b), "integer one of");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i none of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i none of (1, 2)", config);
        mu_assert(eq_expr(a, b), "integer none of");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i all of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i all of (1, 2)", config);
        mu_assert(eq_expr(a, b), "integer all of");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_list_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);

    {
        struct ast_node* a = parse_and_assign("s one of (\"1\", \"2\")", config);
        struct ast_node* b = parse_and_assign("s one of (\"1\", \"2\")", config);
        mu_assert(eq_expr(a, b), "string one of");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("s none of (\"1\", \"2\")", config);
        struct ast_node* b = parse_and_assign("s none of (\"1\", \"2\")", config);
        mu_assert(eq_expr(a, b), "string none of");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("s all of (\"1\", \"2\")", config);
        struct ast_node* b = parse_and_assign("s all of (\"1\", \"2\")", config);
        mu_assert(eq_expr(a, b), "string all of");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_list_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);
    add_attr_domain_i(config, "i2", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("i one of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i one of (\"1\", \"2\")", config);
        mu_assert(!eq_expr(a, b), "wrong value type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i one of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i none of (1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i one of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i2 one of (1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("i one of (1, 2)", config);
        struct ast_node* b = parse_and_assign("i one of (1, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong value");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_special_frequency()
{
    struct config* config = make_default_config();

    {
        struct ast_node* a = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        struct ast_node* b = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        mu_assert(eq_expr(a, b), "within_frequency_cap");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        struct ast_node* b = parse_and_assign("within_frequency_cap(\"product\", \"namespace\", 1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong type");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        struct ast_node* b = parse_and_assign("within_frequency_cap(\"flight\", \"wrong\", 1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong namespace");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        struct ast_node* b = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 2, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong value");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 2)", config);
        struct ast_node* b = parse_and_assign("within_frequency_cap(\"flight\", \"namespace\", 1, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong length");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_special_segment()
{
    struct config* config = make_default_config();

    {
        struct ast_node* a = parse_and_assign("segment_within(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_within(1, 2)", config);
        mu_assert(eq_expr(a, b), "segment_within");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_within(segment, 1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_within(segment, 1, 2)", config);
        mu_assert(eq_expr(a, b), "segment_within with segment");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_before(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_before(1, 2)", config);
        mu_assert(eq_expr(a, b), "segment_before");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_before(segment, 1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_before(segment, 1, 2)", config);
        mu_assert(eq_expr(a, b), "segment_before with segment");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_within(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_within(2, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong id");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_within(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_within(1, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong seconds");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_within(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_before(1, 2)", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("segment_within(1, 2)", config);
        struct ast_node* b = parse_and_assign("segment_within(segment, 1, 2)", config);
        mu_assert(!eq_expr(a, b), "extra var");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_special_geo()
{
    struct config* config = make_default_config();

    {
        struct ast_node* a = parse_and_assign("geo_within_radius(1, 2, 3)", config);
        struct ast_node* b = parse_and_assign("geo_within_radius(1, 2, 3)", config);
        mu_assert(eq_expr(a, b), "geo integer");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("geo_within_radius(1., 2., 3.)", config);
        struct ast_node* b = parse_and_assign("geo_within_radius(1., 2., 3.)", config);
        mu_assert(eq_expr(a, b), "geo float");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("geo_within_radius(1, 2, 3)", config);
        struct ast_node* b = parse_and_assign("geo_within_radius(2, 2, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong latitude");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("geo_within_radius(1, 2, 3)", config);
        struct ast_node* b = parse_and_assign("geo_within_radius(1, 3, 3)", config);
        mu_assert(!eq_expr(a, b), "wrong longitude");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("geo_within_radius(1, 2, 3)", config);
        struct ast_node* b = parse_and_assign("geo_within_radius(1, 2, 4)", config);
        mu_assert(!eq_expr(a, b), "wrong radius");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_special_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);
    add_attr_domain_s(config, "s2", false);

    {
        struct ast_node* a = parse_and_assign("contains(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("contains(s, \"abc\")", config);
        mu_assert(eq_expr(a, b), "contains");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("starts_with(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("starts_with(s, \"abc\")", config);
        mu_assert(eq_expr(a, b), "starts_with");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("ends_with(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("ends_with(s, \"abc\")", config);
        mu_assert(eq_expr(a, b), "ends_with");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("contains(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("starts_with(s, \"abc\")", config);
        mu_assert(!eq_expr(a, b), "wrong op");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("contains(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("contains(s2, \"abc\")", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("contains(s, \"abc\")", config);
        struct ast_node* b = parse_and_assign("contains(s, \"abcd\")", config);
        mu_assert(!eq_expr(a, b), "wrong pattern");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_bool()
{
    struct config* config = make_default_config();
    add_attr_domain_b(config, "b", false, true, false);
    add_attr_domain_i(config, "i", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("b", config);
        struct ast_node* b = parse_and_assign("b", config);
        mu_assert(eq_expr(a, b), "var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("not b", config);
        struct ast_node* b = parse_and_assign("not b", config);
        mu_assert(eq_expr(a, b), "simple not");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("b and b", config);
        struct ast_node* b = parse_and_assign("b and b", config);
        mu_assert(eq_expr(a, b), "simple and");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("b or b", config);
        struct ast_node* b = parse_and_assign("b or b", config);
        mu_assert(eq_expr(a, b), "simple or");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("not (i = 0)", config);
        struct ast_node* b = parse_and_assign("not (i = 0)", config);
        mu_assert(eq_expr(a, b), "complex unary");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("(i = 0) and (i = 0)", config);
        struct ast_node* b = parse_and_assign("(i = 0) and (i = 0)", config);
        mu_assert(eq_expr(a, b), "complex binary");
        free_ast_node(a);
        free_ast_node(b);
    }

    return 0;
}

int test_bool_wrong()
{
    struct config* config = make_default_config();
    add_attr_domain_b(config, "b", false, true, false);
    add_attr_domain_b(config, "b2", false, true, false);
    add_attr_domain_i(config, "i", 0, 10, false);

    {
        struct ast_node* a = parse_and_assign("b", config);
        struct ast_node* b = parse_and_assign("b2", config);
        mu_assert(!eq_expr(a, b), "wrong var");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("not b", config);
        struct ast_node* b = parse_and_assign("not b2", config);
        mu_assert(!eq_expr(a, b), "wrong var not");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("b and b", config);
        struct ast_node* b = parse_and_assign("b or b", config);
        mu_assert(!eq_expr(a, b), "wrong operator");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("not (i = 0)", config);
        struct ast_node* b = parse_and_assign("not b", config);
        mu_assert(!eq_expr(a, b), "complex unary");
        free_ast_node(a);
        free_ast_node(b);
    }
    {
        struct ast_node* a = parse_and_assign("(i = 0) and (i = 0)", config);
        struct ast_node* b = parse_and_assign("(i = 0) and b", config);
        mu_assert(!eq_expr(a, b), "complex binary");
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
    mu_run_test(test_list_integer);
    mu_run_test(test_list_string);
    mu_run_test(test_list_wrong);
    mu_run_test(test_special_frequency);
    mu_run_test(test_special_segment);
    mu_run_test(test_special_geo);
    mu_run_test(test_special_string);
    mu_run_test(test_bool);
    mu_run_test(test_bool_wrong);

    return 0;
}

RUN_TESTS()

