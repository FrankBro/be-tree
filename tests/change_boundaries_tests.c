#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betree.h"
#include "debug.h"
#include "helper.h"
#include "minunit.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

int test_integer()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", false);

    const char* expr1 = "i > 2"; 
    const char* expr2 = "i > 4"; 
    const char* expr3 = "i < 7"; 
    const char* expr4 = "i < 8"; 

    betree_change_boundaries(tree, expr1);
    betree_change_boundaries(tree, expr2);
    betree_change_boundaries(tree, expr3);
    betree_change_boundaries(tree, expr4);

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.imin == 3, "");
    mu_assert(bound.imax == 7, "");

    mu_assert(betree_insert(tree, 1, expr1), "");
    mu_assert(betree_insert(tree, 2, expr2), "");
    mu_assert(betree_insert(tree, 3, expr3), "");
    mu_assert(betree_insert(tree, 4, expr4), "");

    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 3}", report), "");
        mu_assert(report->matched == 3, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 7}", report), "");
        mu_assert(report->matched == 3, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 1}", report), "");
        mu_assert(report->matched == 2, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 9}", report), "");
        mu_assert(report->matched == 2, "");
        free_report(report);
    }

    betree_free(tree);

    return 0;
}

int test_float()
{
    struct betree* tree = betree_make();
    add_attr_domain_f(tree->config, "f", false);

    const char* expr1 = "f > 2."; 
    const char* expr2 = "f > 4."; 
    const char* expr3 = "f < 7."; 
    const char* expr4 = "f < 8."; 

    betree_change_boundaries(tree, expr1);
    betree_change_boundaries(tree, expr2);
    betree_change_boundaries(tree, expr3);
    betree_change_boundaries(tree, expr4);

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(feq(bound.fmin, 2), "");
    mu_assert(feq(bound.fmax, 8), "");

    mu_assert(betree_insert(tree, 1, expr1), "");
    mu_assert(betree_insert(tree, 2, expr2), "");
    mu_assert(betree_insert(tree, 3, expr3), "");
    mu_assert(betree_insert(tree, 4, expr4), "");

    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"f\": 3.}", report), "");
        mu_assert(report->matched == 3, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"f\": 7.}", report), "");
        mu_assert(report->matched == 3, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"f\": 1.}", report), "");
        mu_assert(report->matched == 2, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"f\": 9.}", report), "");
        mu_assert(report->matched == 2, "");
        free_report(report);
    }

    betree_free(tree);

    return 0;
}

int test_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "s", false);

    const char* expr1 = "s = \"2\""; 
    const char* expr2 = "s <> \"4\""; 
    const char* expr3 = "s = \"7\""; 
    const char* expr4 = "s <> \"8\""; 

    betree_change_boundaries(tree, expr1);
    betree_change_boundaries(tree, expr2);
    betree_change_boundaries(tree, expr3);
    betree_change_boundaries(tree, expr4);

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.smin == 0, "");
    mu_assert(bound.smax == 3, "");

    mu_assert(betree_insert(tree, 1, expr1), "");
    mu_assert(betree_insert(tree, 2, expr2), "");
    mu_assert(betree_insert(tree, 3, expr3), "");
    mu_assert(betree_insert(tree, 4, expr4), "");

    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"s\": \"3\"}", report), "");
        mu_assert(report->matched == 2, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"s\": \"4\"}", report), "");
        mu_assert(report->matched == 1, "");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"s\": \"2\"}", report), "");
        mu_assert(report->matched == 3, "");
        free_report(report);
    }

    betree_free(tree);

    return 0;
}

int test_integer_set_left()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", false);

    const char* expr1 = "i in (3,8,2,6)"; 
    /*const char* expr2 = "i in (3,8,2,6)"; */

    betree_change_boundaries(tree, expr1);
    /*betree_change_boundaries(tree, expr2);*/

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.imin == 2, "");
    mu_assert(bound.imax == 8, "");

    betree_free(tree);

    return 0;
}

int test_integer_set_right()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "il", false);

    const char* expr1 = "3 in il"; 
    /*const char* expr2 = "i in (3,8,2,6)"; */

    betree_change_boundaries(tree, expr1);
    /*betree_change_boundaries(tree, expr2);*/

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.imin == 3, "");
    mu_assert(bound.imax == 3, "");

    betree_free(tree);

    return 0;
}

int test_integer_list()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "il", false);

    const char* expr1 = "il one of (2,8,4,1)"; 
    /*const char* expr2 = "i in (3,8,2,6)"; */

    betree_change_boundaries(tree, expr1);
    /*betree_change_boundaries(tree, expr2);*/

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.imin == 1, "");
    mu_assert(bound.imax == 8, "");

    betree_free(tree);

    return 0;
}

int test_normal()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", false);

    const char* expr1 = "i = 1"; 
    const char* expr2 = "i = 2"; 
    const char* expr3 = "i = 3"; 
    const char* expr4 = "i = 4"; 
    const char* expr5 = "i = 5"; 

    betree_change_boundaries(tree, expr1);
    betree_change_boundaries(tree, expr2);
    betree_change_boundaries(tree, expr3);
    betree_change_boundaries(tree, expr4);
    betree_change_boundaries(tree, expr5);

    struct value_bound bound = tree->config->attr_domains[0]->bound;
    mu_assert(bound.imin == 1, "");
    mu_assert(bound.imax == 5, "");

    mu_assert(betree_insert(tree, 1, expr1), "");
    mu_assert(betree_insert(tree, 2, expr2), "");
    mu_assert(betree_insert(tree, 3, expr3), "");
    mu_assert(betree_insert(tree, 4, expr4), "");
    mu_assert(betree_insert(tree, 5, expr5), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"i\": 2}", report), "");

    mu_assert(report->evaluated != 5 && report->matched == 1, "");

    free_report(report);
    betree_free(tree);

    return 0;
}

int all_tests()
{
    mu_run_test(test_integer);
    mu_run_test(test_float);
    mu_run_test(test_string);
    mu_run_test(test_integer_set_left);
    mu_run_test(test_integer_set_right);
    mu_run_test(test_normal);

    return 0;
}

RUN_TESTS()

