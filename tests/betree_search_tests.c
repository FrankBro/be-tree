#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "betree.h"
#include "debug.h"
#include "helper.h"
#include "minunit.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

int test_search()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 0);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 1);

    mu_assert(betree_insert(tree, 1, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 1"), "");
    mu_assert(betree_insert(tree, 3, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 4, "a = 0 and b = 1"), "");
    mu_assert(betree_insert(tree, 5, "b = 1 and a = 0"), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"a\": 0, \"b\": 1}", report), "");
    mu_assert(report->matched == 2 && report->subs[0] == 4 && report->subs[1] == 5, "goodEvent");

    write_dot_to_file(tree, "tests/beetree_search_tests.dot");

    free_report(report);
    betree_free(tree);
    return 0;
}
int test_search_ids()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 0);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 1);

    mu_assert(betree_insert(tree, 1, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 1"), "");
    mu_assert(betree_insert(tree, 3, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 4, "a = 0 and b = 1"), "");
    mu_assert(betree_insert(tree, 5, "b = 1 and a = 0"), "");

    struct report* report = make_report();
    uint64_t ids[] = {1, 4};
    size_t sz = 2;
    mu_assert(betree_search_ids(tree, "{\"a\": 0, \"b\": 1}", report, ids, sz), "");
    mu_assert(report->matched == 1 && report->subs[0] == 4, "goodEvent");

    write_dot_to_file(tree, "tests/beetree_search_tests.dot");

    free_report(report);
    return 0;
}

int all_tests()
{
    mu_run_test(test_search);
    mu_run_test(test_search_ids);
    return 0;
}

RUN_TESTS()
