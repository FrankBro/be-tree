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

int test_complex() 
{
    struct betree* tree = betree_make();
    add_attr_domain_b(tree->config, "private", false);
    add_attr_domain_i(tree->config, "exchange", false);
    add_attr_domain_i(tree->config, "member_id", false);
    add_attr_domain_i(tree->config, "exchange_seller_site_id", false);
    add_attr_domain_i(tree->config, "publisher_id", false);
    add_attr_domain_s(tree->config, "country", false);
    add_attr_domain_s(tree->config, "region", false);
    add_attr_domain_il(tree->config, "iab_categories", false);
    add_attr_domain_il(tree->config, "ias_segments", false);
    add_attr_domain_s(tree->config, "impression_type", false);
    add_attr_domain_il(tree->config, "sitelist_ids", false);
    add_attr_domain_i(tree->config, "device_type_id", false);
    add_attr_domain_il(tree->config, "exchange_seat_ids", false);

    const char* expr1 = "(((not private)) and (((exchange <> 6 or exchange = 6 and member_id <> 22 or exchange = 6 and member_id = 22 and exchange_seller_site_id <> 1145) and (exchange <> 6 or exchange = 6 and member_id <> 22 or exchange = 6 and member_id = 22 and exchange_seller_site_id <> 1146))) and (((exchange <> 6 or exchange = 6 and member_id <> 22 or exchange = 6 and member_id = 22 and publisher_id = 68) or (exchange <> 6 or exchange = 6 and member_id <> 22 or exchange = 6 and member_id = 22 and publisher_id = 69))))";
    const char* expr2 = "((((not private)) and ((country = \"CA\" and region = \"QC\")) and ((exchange not in (2,5,14,16,7,9,3,11) or exchange in (2,5,14,16,7,9,3,11) and iab_categories none of (240000,260000,250000))) and (ias_segments all of (502,504,506,508,510,512,532)) and (((impression_type <> 'site' or sitelist_ids none of (637)) and (impression_type <> 'site' or sitelist_ids one of (2200)))) and ((device_type_id = 3 or device_type_id = 1)) and ((segment_within(20198, 604800) and not (segment_within(20202, 604800) or segment_within(20192, 604800))))) and (exchange <> 4 or exchange = 4 and 2 in exchange_seat_ids)) and ((within_frequency_cap(\"campaign\", \"849922\", 24, 2592000) and within_frequency_cap(\"campaign\", \"853821\", 1, 60))) and (within_frequency_cap(\"advertiser\", \"3049521\", 15, 86400))";

    betree_change_boundaries(tree, expr1);
    betree_change_boundaries(tree, expr2);

    for(size_t i = 0; i < tree->config->attr_domain_count; i++) {
        struct attr_domain* domain = tree->config->attr_domains[i];
        print_attr_domain(domain);
    }

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

    fprintf(stderr, "DEBUG: evaluated = %zu\n", report->evaluated);
    mu_assert(report->evaluated != 5 && report->matched == 1, "");

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
    mu_run_test(test_complex);
    mu_run_test(test_normal);

    return 0;
}

RUN_TESTS()

