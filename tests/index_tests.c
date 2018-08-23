#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betree.h"
#include "debug.h"
#include "helper.h"
#include "minunit.h"
#include "tree.h"
#include "utils.h"

int test_index (bool strict)
{
    // Setup
    struct betree* tree = betree_make_with_parameters(4, 0);
    betree_add_integer_variable(tree, "width", false, INT64_MIN, INT64_MAX);
    betree_add_integer_variable(tree, "height", true, INT64_MIN, INT64_MAX);
    betree_add_string_variable(tree, "type", true, 3);
    betree_add_string_list_variable(tree, "colors", true, 1);

    struct betree_index_definition* creative_index = betree_make_index_definition("creative_index");
    betree_add_string_index(creative_index, "type", strict);
    betree_add_index(tree, creative_index);

    enum count { constant_count = 1 };

    // Insert
    {
        // {1122, [{my_index, [[<<"jpg">>]]}], "width > 600 and type = 'jpg'"},
        struct betree_index_entries* entries = betree_make_index_entries();
        {
            struct betree_index_entry* entry = betree_make_index_entry(1);
            betree_add_index_string_value(entry, "jpg");
            betree_add_index_entry(entries, entry);
        }
        struct betree_expression expr = {
            .id = 1122,
            .constant_count = 0,
            .constants = NULL,
            .index_count = 1,
            .indexes = calloc(1, sizeof(*expr.indexes)),
            .expr = "width > 600 and type = \"jpg\""
        };
        expr.indexes[0] = betree_make_index("creative_index", entries);
        mu_assert(betree_insert_with_struct(tree, &expr), "");
    }
    {
        // {3344, [{my_index, [[undefined]]}], "width > 600 and type = 'jpg'"},
        struct betree_index_entries* entries = betree_make_index_entries();
        {
            struct betree_index_entry* entry = betree_make_index_entry(1);
            betree_add_index_undefined_value(entry);
            betree_add_index_entry(entries, entry);
        }
        struct betree_expression expr = {
            .id = 3344,
            .constant_count = 0,
            .constants = NULL,
            .index_count = 1,
            .indexes = calloc(1, sizeof(*expr.indexes)),
            .expr = "width > 600 and type = \"jpg\""
        };
        expr.indexes[0] = betree_make_index("creative_index", entries);
        mu_assert(betree_insert_with_struct(tree, &expr), "");
    }
    {
        // {6677, [{my_index, [[<<"jpg">>]]}], "height = 320 or 'blue' in colors"},
        struct betree_index_entries* entries = betree_make_index_entries();
        {
            struct betree_index_entry* entry = betree_make_index_entry(1);
            betree_add_index_string_value(entry, "jpg");
            betree_add_index_entry(entries, entry);
        }
        struct betree_expression expr = {
            .id = 6677,
            .constant_count = 0,
            .constants = NULL,
            .index_count = 1,
            .indexes = calloc(1, sizeof(*expr.indexes)),
            .expr = "height = 320 or \"blue\" in colors"
        };
        expr.indexes[0] = betree_make_index("creative_index", entries);
        mu_assert(betree_insert_with_struct(tree, &expr), "");
    }
    {
        // {8899, [{my_index, [[undefined]]}], "height = 320 or 'blue' in colors"}
        struct betree_index_entries* entries = betree_make_index_entries();
        {
            struct betree_index_entry* entry = betree_make_index_entry(1);
            betree_add_index_undefined_value(entry);
            betree_add_index_entry(entries, entry);
        }
        struct betree_expression expr = {
            .id = 8899,
            .constant_count = 0,
            .constants = NULL,
            .index_count = 1,
            .indexes = calloc(1, sizeof(*expr.indexes)),
            .expr = "height = 320 or \"blue\" in colors"
        };
        expr.indexes[0] = betree_make_index("creative_index", entries);
        mu_assert(betree_insert_with_struct(tree, &expr), "");
    }

    // Search
    {
        // {png,   rtb_boolean:evaluate(Handle, [{xx, 800, 320, <<"png">>, [<<"blue">>]}])},
        struct betree_event* event = betree_make_event(tree);
        betree_set_variable(event, 0, betree_make_integer_variable("width", 800));
        betree_set_variable(event, 1, betree_make_integer_variable("height", 320));
        betree_set_variable(event, 2, betree_make_string_variable("type", "png"));
        struct betree_string_list* sl = betree_make_string_list(1);
        betree_add_string(sl, 0, "blue");
        betree_set_variable(event, 3, betree_make_string_list_variable("colors", sl));

        struct report* report = make_report();
        betree_search_with_event(tree, event, report);

        if(strict) {
            mu_assert(report->matched == 0, "png: found correct amount");
        }
        else {
            mu_assert(report->matched == 1, "png: found correct amount");
            mu_assert(report->subs[0] == 8899, "png: found correct subs");
        }

        free_report(report);
    }
    {
        // {jpg,   rtb_boolean:evaluate(Handle, [{xx, 800, 320, <<"jpg">>, [<<"blue">>]}])},
        struct betree_event* event = betree_make_event(tree);
        betree_set_variable(event, 0, betree_make_integer_variable("width", 800));
        betree_set_variable(event, 1, betree_make_integer_variable("height", 320));
        betree_set_variable(event, 2, betree_make_string_variable("type", "jpg"));
        struct betree_string_list* sl = betree_make_string_list(1);
        betree_add_string(sl, 0, "blue");
        betree_set_variable(event, 3, betree_make_string_list_variable("colors", sl));

        struct report* report = make_report();
        betree_search_with_event(tree, event, report);

        if(strict) {
            mu_assert(report->matched == 2, "jpg: found correct amount");
            mu_assert(report->subs[0] == 6677 && report->subs[1] == 1122, "jpg: found correct subs");
        }
        else {
            mu_assert(report->matched == 4, "jpg: found correct amount");
            mu_assert(report->subs[0] == 8899 && report->subs[1] == 6677 &&
                      report->subs[2] == 3344 && report->subs[3] == 1122, "jpg: found correct subs");
        }

        free_report(report);
    }
    {
        // {undef, rtb_boolean:evaluate(Handle, [{xx, 800, 320, undefined, [<<"blue">>]}])}
        struct betree_event* event = betree_make_event(tree);
        betree_set_variable(event, 0, betree_make_integer_variable("width", 800));
        betree_set_variable(event, 1, betree_make_integer_variable("height", 320));
        betree_set_variable(event, 2, NULL);
        struct betree_string_list* sl = betree_make_string_list(1);
        betree_add_string(sl, 0, "blue");
        betree_set_variable(event, 3, betree_make_string_list_variable("colors", sl));

        struct report* report = make_report();
        betree_search_with_event(tree, event, report);

        if(strict) {
            mu_assert(report->matched == 1, "undef: found correct amount");
            mu_assert(report->subs[0] == 8899, "undef: found correct subs");
        }
        else {
            mu_assert(report->matched == 1, "undef: found correct amount");
            mu_assert(report->subs[0] == 8899, "undef: found correct subs");
        }

        free_report(report);
    }

    // strict
    // [{png,[]},{jpg,[6677,1122]},{undef,[8899]}]

    // not_strict
    // [{png,[8899]},{jpg,[8899,6677,3344,1122]},{undef,[8899]}]

    betree_free(tree);

    return 0;
}

int test_index_strict()
{
    return test_index(true);
}

int test_index_not_strict()
{
    return test_index(false);
}

int all_tests()
{
    mu_run_test(test_index_strict);
    mu_run_test(test_index_not_strict);

    return 0;
}

RUN_TESTS()
