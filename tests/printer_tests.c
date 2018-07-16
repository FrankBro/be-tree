#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "minunit.h"
#include "printer.h"

int parse(const char *text, struct ast_node **node);

bool parse_and_compare(const char* expr)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        return false;
    }
    char* printed = ast_to_string(node);
    bool result = strcmp(expr, printed) == 0;
    if(result == false) {
        printf("%s != %s\n", expr, printed);
    }
    free(printed);
    return result;
}

int test_compare()
{
    mu_assert(parse_and_compare("i > 1"), "gt int");
    mu_assert(parse_and_compare("i >= 1"), "ge int");
    mu_assert(parse_and_compare("i < 1"), "lt int");
    mu_assert(parse_and_compare("i <= 1"), "le int");
    mu_assert(parse_and_compare("i > -1"), "negative int");

    mu_assert(parse_and_compare("f > 1.00"), "gt float");
    mu_assert(parse_and_compare("f >= 1.00"), "ge float");
    mu_assert(parse_and_compare("f < 1.00"), "lt float");
    mu_assert(parse_and_compare("f <= 1.00"), "le float");
    mu_assert(parse_and_compare("f > -1.00"), "negative float");

    return 0;
}

int test_equality()
{
    mu_assert(parse_and_compare("i = 1"), "eq int");
    mu_assert(parse_and_compare("i <> 1"), "ne int");
    mu_assert(parse_and_compare("i = -1"), "negative int");

    mu_assert(parse_and_compare("f = 1.00"), "eq float");
    mu_assert(parse_and_compare("f <> 1.00"), "ne float");
    mu_assert(parse_and_compare("f = -1.00"), "negative float");

    mu_assert(parse_and_compare("s = \"1\""), "eq string");
    mu_assert(parse_and_compare("s <> \"1\""), "ne string");

    return 0;
}

int test_set()
{
    mu_assert(parse_and_compare("i in (1, 2, 3)"), "in int");
    mu_assert(parse_and_compare("i not in (1, 2, 3)"), "in int");
    mu_assert(parse_and_compare("i in (-1)"), "negative int");

    mu_assert(parse_and_compare("s in (\"1\", \"2\", \"3\")"), "in string");
    mu_assert(parse_and_compare("s not in (\"1\", \"2\", \"3\")"), "in string");

    mu_assert(parse_and_compare("1 in il"), "in int list");
    mu_assert(parse_and_compare("1 not in il"), "not in int list");
    mu_assert(parse_and_compare("-1 in il"), "negative int list");

    mu_assert(parse_and_compare("\"1\" in sl"), "in string list");
    mu_assert(parse_and_compare("\"1\" not in sl"), "not in string list");

    return 0;
}

int test_list()
{
    mu_assert(parse_and_compare("il one of (1, 2, 3)"), "one of int");
    mu_assert(parse_and_compare("il none of (1, 2, 3)"), "none of int");
    mu_assert(parse_and_compare("il all of (1, 2, 3)"), "all of int");
    mu_assert(parse_and_compare("il one of (-1)"), "negative int");

    mu_assert(parse_and_compare("sl one of (\"1\", \"2\", \"3\")"), "one of string");
    mu_assert(parse_and_compare("sl none of (\"1\", \"2\", \"3\")"), "none of string");
    mu_assert(parse_and_compare("sl all of (\"1\", \"2\", \"3\")"), "all of string");

    return 0;
}

int test_bool()
{
    mu_assert(parse_and_compare("b"), "var");
    mu_assert(parse_and_compare("(not (b))"), "not");
    mu_assert(parse_and_compare("((b) and (b))"), "and");
    mu_assert(parse_and_compare("((b) or (b))"), "or");

    mu_assert(parse_and_compare("((i = 0) and ((not (i > 9))))"), "complex 1");

    return 0;
}

int test_special()
{
    mu_assert(parse_and_compare("within_frequency_cap(\"flight\", \"ns\", 1, 2)"), "frequency");

    mu_assert(parse_and_compare("segment_within(segment, 1, 2)"), "segment within");
    mu_assert(parse_and_compare("segment_before(segment, 1, 2)"), "segment within");

    mu_assert(parse_and_compare("geo_within_radius(1.00, 2.00, 3.00)"), "geo");

    mu_assert(parse_and_compare("contains(var, \"string\")"), "contains");
    mu_assert(parse_and_compare("starts_with(var, \"string\")"), "starts_with");
    mu_assert(parse_and_compare("ends_with(var, \"string\")"), "ends_with");

    return 0;
}

int all_tests()
{
    mu_run_test(test_compare);
    mu_run_test(test_equality);
    mu_run_test(test_set);
    mu_run_test(test_list);
    mu_run_test(test_bool);
    mu_run_test(test_special);

    return 0;
}

RUN_TESTS()
