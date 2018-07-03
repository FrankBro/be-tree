#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "hashmap.h"
#include "parser.h"
#include "memoize.h"
#include "minunit.h"
#include "utils.h"
#include "value.h"
#include "var.h"

extern bool MATCH_NODE_DEBUG;

struct report* test(const char* expr_a, const char* expr_b, const char* event, struct betree* tree)
{
    if(tree->config->pred_map == NULL) {
        tree->config->pred_map = make_pred_map();
    }
    betree_insert(tree, 1, expr_a);
    betree_insert(tree, 2, expr_b);
    struct report* report = make_report();
    betree_search(tree, event, report);
    free_cnode(tree->cnode);
    tree->cnode = make_cnode(tree->config, NULL);
    free_pred_map(tree->config->pred_map);
    tree->config->pred_map = NULL;
    return report;
}

bool test_same(const char* expr, const char* event, struct betree* tree)
{
    struct report* report = test(expr, expr, event, tree);
    bool result =
        report->evaluated == 2 &&
        report->matched == 2 &&
        report->memoized == 1;
    free_report(report);
    return result;
}

bool test_diff(const char* expr_a, const char* expr_b, const char* event, struct betree* tree)
{
    struct report* report = test(expr_a, expr_b, event, tree);
    bool result =
        report->evaluated == 2 &&
        report->matched == 2 &&
        report->memoized == 0;
    free_report(report);
    return result;
}

bool test_same1(size_t memoized, const char* expr, const char* event, struct betree* tree)
{
    struct report* report = test(expr, expr, event, tree);
    bool result =
        report->evaluated == 2 &&
        report->matched == 2 &&
        report->memoized == memoized;
    free_report(report);
    return result;
}

bool test_same2(size_t memoized, const char* expr_a, const char* expr_b, const char* event, struct betree* tree)
{
    struct report* report = test(expr_a, expr_b, event, tree);
    bool result =
        report->evaluated == 2 &&
        report->matched == 2 &&
        report->memoized == memoized;
    free_report(report);
    return result;
}

int test_numeric_compare_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", 0, 10, false);

    mu_assert(test_same("i > 1", "{\"i\": 2}", tree), "integer gt");
    mu_assert(test_same("i >= 1", "{\"i\": 2}", tree), "integer ge");
    mu_assert(test_same("i < 1", "{\"i\": 0}", tree), "integer lt");
    mu_assert(test_same("i <= 1", "{\"i\": 1}", tree), "integer le");
    mu_assert(test_diff("i > 0", "i > 1", "{\"i\": 2}", tree), "integer diff");

    betree_free(tree);
    return 0;
}

int test_numeric_compare_float()
{
    struct betree* tree = betree_make();
    add_attr_domain_f(tree->config, "f", 0., 10., false);

    mu_assert(test_same("f > 1.", "{\"f\": 2.}", tree), "float gt");
    mu_assert(test_same("f >= 1.", "{\"f\": 2.}", tree), "float ge");
    mu_assert(test_same("f < 1.", "{\"f\": 0.}", tree), "float lt");
    mu_assert(test_same("f <= 1.", "{\"f\": 1.}", tree), "float le");
    mu_assert(test_diff("f > 0.", "f > 1.", "{\"f\": 2.}", tree), "float diff");

    betree_free(tree);
    return 0;
}

int test_equality_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", 0, 10, false);

    mu_assert(test_same("i = 1", "{\"i\": 1}", tree), "integer eq");
    mu_assert(test_same("i <> 1", "{\"i\": 0}", tree), "integer ne");
    mu_assert(test_diff("i <> 0", "i <> 1", "{\"i\": 2}", tree), "integer diff");

    betree_free(tree);
    return 0;
}

int test_equality_float()
{
    struct betree* tree = betree_make();
    add_attr_domain_f(tree->config, "f", 0., 10., false);

    mu_assert(test_same("f = 1.", "{\"f\": 1.}", tree), "float eq");
    mu_assert(test_same("f <> 1.", "{\"f\": 0.}", tree), "float ne");
    mu_assert(test_diff("f <> 0.", "f <> 1.", "{\"f\": 2.}", tree), "float diff");

    betree_free(tree);
    return 0;
}

int test_equality_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "s", false);

    mu_assert(test_same("s = \"a\"", "{\"s\": \"a\"}", tree), "string eq");
    mu_assert(test_same("s <> \"a\"", "{\"s\": \"b\"}", tree), "string ne");
    mu_assert(test_diff("s <> \"a\"", "s <> \"b\"", "{\"s\": \"c\"}", tree), "string diff");

    betree_free(tree);
    return 0;
}

int test_set_var_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", 0, 10, false);

    mu_assert(test_same("i in (1,2)", "{\"i\": 1}", tree), "integer set var in");
    mu_assert(test_same("i not in (1,2)", "{\"i\": 3}", tree), "integer set var not in");
    mu_assert(test_diff("i in (1, 3)", "i in (1, 2)", "{\"i\": 1}", tree), "integer set var diff");

    betree_free(tree);
    return 0;
}

int test_set_var_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "s", false);

    mu_assert(test_same("s in (\"1\",\"2\")", "{\"s\": \"1\"}", tree), "string set var in");
    mu_assert(test_same("s not in (\"1\",\"2\")", "{\"s\": \"3\"}", tree), "string set var not in");
    mu_assert(test_diff("s in (\"1\",\"3\")", "s in (\"1\",\"2\")", "{\"s\": \"1\"}", tree), "string set var diff");

    betree_free(tree);
    return 0;
}

int test_set_list_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "il", false);

    mu_assert(test_same("1 in il", "{\"il\": [1, 2]}", tree), "integer set list in");
    mu_assert(test_same("1 not in il", "{\"il\": [2, 3]}", tree), "integer set list not in");
    mu_assert(test_diff("1 in il", "2 in il", "{\"il\": [1, 2]}", tree), "integer set list diff");

    betree_free(tree);
    return 0;
}

int test_set_list_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_sl(tree->config, "sl", false);

    mu_assert(test_same("\"1\" in sl", "{\"sl\": [\"1\", \"2\"]}", tree), "string set list in");
    mu_assert(test_same("\"1\" not in sl", "{\"sl\": [\"2\", \"3\"]}", tree), "string set list not in");
    mu_assert(test_diff("\"1\" in sl", "\"2\" in sl", "{\"sl\": [\"1\", \"2\"]}", tree), "string set list diff");

    betree_free(tree);
    return 0;
}

int test_list_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "il", false);

    mu_assert(test_same("il one of (1, 2)", "{\"il\": [1, 2]}", tree), "integer list one of");
    mu_assert(test_same("il none of (1, 2)", "{\"il\": [3, 4]}", tree), "integer list none of");
    mu_assert(test_same("il all of (1, 2)", "{\"il\": [1, 2]}", tree), "integer list all of");
    mu_assert(test_diff("il one of (1, 2)", "il one of (1, 3)", "{\"il\": [1, 2]}", tree), "integer list diff");

    betree_free(tree);
    return 0;
}

int test_list_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_sl(tree->config, "sl", false);

    mu_assert(test_same("sl one of (\"1\", \"2\")", "{\"sl\": [\"1\", \"2\"]}", tree), "string list one of");
    mu_assert(test_same("sl none of (\"1\", \"2\")", "{\"sl\": [\"3\", \"4\"]}", tree), "string list none of");
    mu_assert(test_same("sl all of (\"1\", \"2\")", "{\"sl\": [\"1\", \"2\"]}", tree), "string list all of");
    mu_assert(test_diff("sl one of (\"1\", \"2\")", "sl one of (\"1\", \"3\")", "{\"sl\": [\"1\", \"2\"]}", tree), "string list diff");

    betree_free(tree);
    return 0;
}

int test_special_frequency()
{
    //struct betree* tree = betree_make();

    // "within_frequency_cap(\"flight\", \"namespace\", 1, 2)"

    return 0;
}

int test_special_segment()
{
    //struct betree* tree = betree_make();

    // "segment_within(1, 2)"
    // "segment_within(segment, 1, 2)"
    // "segment_before(1, 2)"
    // "segment_before(segment, 1, 2)"

    return 0;
}

int test_special_geo()
{
    //struct betree* tree = betree_make();

    // "geo_within_radius(1, 2, 3)"
    // "geo_within_radius(1., 2., 3.)"

    return 0;
}

int test_special_string()
{
    //struct betree* tree = betree_make();
    //add_attr_domain_s(config, "s", false);
    //add_attr_domain_s(config, "s2", false);

    // "contains(s, \"abc\")"
    // "starts_with(s, \"abc\")"
    // "ends_with(s, \"abc\")"

    return 0;
}

int test_bool()
{
    struct betree* tree = betree_make();
    add_attr_domain_b(tree->config, "b", true);
    add_attr_domain_i(tree->config, "i", 0, 10, true);

    mu_assert(test_same("b", "{\"b\": true}", tree), "bool var");
    mu_assert(test_same("not b", "{\"b\": false}", tree), "bool not");
    mu_assert(test_same1(2, "b and b", "{\"b\": true}", tree), "bool and");
    mu_assert(test_same("b or b", "{\"b\": true}", tree), "bool or");
    mu_assert(test_same("not (i = 0)", "{\"i\": 1}", tree), "bool not complex");
    mu_assert(test_same1(2, "(i = 0) and (i = 0)", "{\"i\": 0}", tree), "bool and complex");
    mu_assert(test_diff("(i = 0) or (i = 1)", "(i <> 1) or (i = 2)", "{\"i\": 0}", tree), "bool diff");

    betree_free(tree);
    return 0;
}

int test_sub()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", 0, 10, true);
    add_attr_domain_b(tree->config, "b", true);
    add_attr_domain_il(tree->config, "il", true);
    add_attr_domain_sl(tree->config, "sl", true);

    mu_assert(test_same2(1, "(i = 0) or (i = 1)", "(i = 0) or (i = 2)", "{\"i\": 0}", tree), "");
    mu_assert(test_same2(1, 
        "((((not b) or (i = 6 and (\"s2\" in sl)))) and (il one of (2, 3)))",
        "((((not b) or (i = 6 and (\"s2\" in sl)))) and (il one of (2, 4)))",
        "{\"b\": false, \"i\": 6, \"sl\": [\"s1\",\"s2\"], \"il\": [1, 2]}",
        tree), "whole left side of and");

    betree_free(tree);
    return 0;
}

int test_bit_logic()
{
    enum { pred_count = 250 };
    struct memoize memoize = make_memoize(pred_count);
    for(size_t i = 0; i < pred_count; i++) {
        if(i % 2 == 0) {
            set_bit(memoize.result, i);
        }
        for(size_t j = 0; j < pred_count; j++) {
            if(j <= i) {
                if(j % 2 == 0) {
                    mu_assert(test_bit(memoize.result, j), "even, pass for %zu, at %zu", j, i);
                }
                else {
                    mu_assert(!test_bit(memoize.result, j), "odd, pass for %zu, at %zu", j, i);
                }
            }
            else {
                mu_assert(!test_bit(memoize.result, j), "new, pass for %zu, at %zu", j, i);
            }
        }
    }
    free_memoize(memoize);
    return 0;
}

int all_tests() 
{
    mu_run_test(test_numeric_compare_integer);
    mu_run_test(test_numeric_compare_float);
    mu_run_test(test_equality_integer);
    mu_run_test(test_equality_float);
    mu_run_test(test_equality_string);
    mu_run_test(test_set_var_integer);
    mu_run_test(test_set_var_string);
    mu_run_test(test_set_list_integer);
    mu_run_test(test_set_list_string);
    mu_run_test(test_list_integer);
    mu_run_test(test_list_string);
    mu_run_test(test_special_frequency);
    mu_run_test(test_special_segment);
    mu_run_test(test_special_geo);
    mu_run_test(test_special_string);
    mu_run_test(test_bool);
    mu_run_test(test_sub);
    mu_run_test(test_bit_logic);

    return 0;
}

RUN_TESTS()


