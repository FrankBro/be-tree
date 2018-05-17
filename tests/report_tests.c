#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "betree.h"
#include "minunit.h"

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct event** event);

int test_integer()
{
    struct config* config = make_config(1, 0);
    add_attr_domain_i(config, "a", 0, 10, false);

    {
        struct report report = make_empty_report();
        struct cnode* cnode = make_cnode(config, NULL);
        betree_insert(config, 1, "a > 6", cnode);
        betree_insert(config, 2, "a < 6", cnode);
        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\":2}", cnode, matched_subs, &report);
        mu_assert(matched_subs->sub_count == 1, "");
        mu_assert(report.expressions_evaluated == 1 && report.expressions_matched == 1, "");
        free_cnode(cnode);
        free_matched_subs(matched_subs);
    }

    free_config(config);

    return 0;
}

int all_tests()
{
    mu_run_test(test_integer);

    return 0;
}

RUN_TESTS()
