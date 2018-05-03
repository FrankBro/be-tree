#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"
#include "utils.h"

int parse(const char *text, struct ast_node **node);

static bool geo(bool has_not, const char* latitude, const char* longitude, const char* radius, double latitude_value, double longitude_value)
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "latitude", 0.0, 10.0, false);
    add_attr_domain_f(config, "longitude", 0.0, 10.0, false);
    struct ast_node* node = NULL;
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    asprintf(&expr, "%sgeo_within_radius(%s, %s, %s)", pre, latitude, longitude, radius);
    parse(expr, &node);
    struct event* event = (struct event*)make_event();
    event->pred_count = 2;
    event->preds = calloc(2, sizeof(*event->preds));
    event->preds[0] = (struct pred*)make_simple_pred_f(0, latitude_value);
    event->preds[1] = (struct pred*)make_simple_pred_f(1, longitude_value);
    bool result = match_node(config, event, node);
    free_config(config);
    free_ast_node(node);
    free_event((struct event*)event);
    return result;
}

static bool geo_within_radius(const char* latitude, const char* longitude, const char* radius, double latitude_value, double longitude_value) { return geo(false, latitude, longitude, radius, latitude_value, longitude_value); }
static bool not_geo_within_radius(const char* latitude, const char* longitude, const char* radius, double latitude_value, double longitude_value) { return geo(true, latitude, longitude, radius, latitude_value, longitude_value); }

int test_geo()
{
    mu_assert(geo_within_radius("100", "100", "10", 100.0, 100.0), "geo_within_int_inside");
    mu_assert(!geo_within_radius("100", "100", "10", 200.0, 200.0), "geo_within_int_outside");
    mu_assert(geo_within_radius("100.0", "100.0", "10.0", 100.0, 100.0), "geo_within_float_inside");
    mu_assert(!geo_within_radius("100.0", "100.0", "10.0", 200.0, 200.0), "geo_within_float_outside");
    // mu_assert(!not_geo_within_radius("100.0", "100.0", "10.0", 100.0, 100.0), "geo_not_within_float_inside");
    // mu_assert(not_geo_within_radius("100.0", "100.0", "10.0", 200.0, 200.0), "geo_not_within_float_outside");
    return 0;
}

static bool contains(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, attr, allow_undefined);
    struct ast_node* node = NULL;
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    asprintf(&expr, "%scontains(%s, \"%s\")", pre, attr, pattern);
    parse(expr, &node);
    const char* event_attr = allow_undefined ? "a" : attr;
    const struct event* event = make_simple_event_s(config, event_attr, value);
    bool result = match_node(config, event, node);
    free_config(config);
    free_ast_node(node);
    free_event((struct event*)event);
    return result;
}

static bool contains_a(const char* pattern, const char* value) { return contains(false, "a", false, pattern, value); }
static bool contains_b(const char* pattern, const char* value) { return contains(false, "b", true, pattern, value); }
static bool not_contains_a(const char* pattern, const char* value) { return contains(true, "a", false, pattern, value); }

static int test_contains()
{
    mu_assert(contains_a("", "abc"), "contains_in_nil");
    mu_assert(contains_a("abc", "aabcc"), "contains_in_mid");
    mu_assert(contains_a("abc", "abcc"), "contains_in_start");
    mu_assert(contains_a("abc", "aabc"), "contains_in_end");
    mu_assert(contains_a("abc", "abc"), "contains_in_exact");
    mu_assert(!contains_a("abc", "aabbcc"), "contains_out_var_bigger");
    mu_assert(!contains_a("abc", "a"), "contains_out_var_smaller");
    mu_assert(!contains_a("abc", ""), "contains_out_var_nil");
    mu_assert(!contains_a("aabbcc", "abc"), "contains_out_pattern_bigger");
    mu_assert(!contains_a("z", "abc"), "contains_out_pattern_smaller");
    mu_assert(!contains_b("abc", "abc"), "contains_out_undef");
    // mu_assert(!not_contains_a("abc", "aabcc"), "contains_not_mid");
    // mu_assert(!not_contains_a("abc", "abc"), "contains_not_exact");
    // mu_assert(not_contains_a("abc", "aabbcc"), "contains_not_var_bigger");
    // mu_assert(not_contains_a("z", "abc"), "contains_not_pattern_smaller");
    return 0;
}

static bool starts_with(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, attr, allow_undefined);
    struct ast_node* node = NULL;
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    asprintf(&expr, "%sstarts_with(%s, \"%s\")", pre, attr, pattern);
    parse(expr, &node);
    const char* event_attr = allow_undefined ? "a" : attr;
    const struct event* event = make_simple_event_s(config, event_attr, value);
    bool result = match_node(config, event, node);
    free_config(config);
    free_ast_node(node);
    free_event((struct event*)event);
    return result;
}

static bool starts_with_a(const char* pattern, const char* value) { return starts_with(false, "a", false, pattern, value); }
static bool starts_with_b(const char* pattern, const char* value) { return starts_with(false, "b", true, pattern, value); }
static bool not_starts_with_a(const char* pattern, const char* value) { return starts_with(true, "a", false, pattern, value); }

static int test_starts_with()
{
    mu_assert(starts_with_a("", "abc"), "starts_with_in_nil");
    mu_assert(starts_with_a("abc", "abcc"), "starts_with_in_start");
    mu_assert(starts_with_a("abc", "abc"), "starts_with_in_exact");
    mu_assert(!starts_with_a("abc", "a"), "starts_with_out_var_smaller");
    mu_assert(!starts_with_a("abc", ""), "starts_with_out_var_nil");
    mu_assert(!starts_with_b("abc", "abc"), "starts_with_out_undef");
    // mu_assert(!not_starts_with_a("abc", "abcc"), "starts_with_not_start");
    // mu_assert(!not_starts_with_a("abc", "abc"), "starts_with_not_exact");
    // mu_assert(not_starts_with_a("abc", "a"), "starts_with_not_var_smaller");
    // mu_assert(not_starts_with_a("abc", "", "starts_with_not_var_nil");
    return 0;
}

static bool ends_with(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, attr, allow_undefined);
    struct ast_node* node = NULL;
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    asprintf(&expr, "%sends_with(%s, \"%s\")", pre, attr, pattern);
    parse(expr, &node);
    const char* event_attr = allow_undefined ? "a" : attr;
    const struct event* event = make_simple_event_s(config, event_attr, value);
    bool result = match_node(config, event, node);
    free_config(config);
    free_ast_node(node);
    free_event((struct event*)event);
    return result;
}

static bool ends_with_a(const char* pattern, const char* value) { return ends_with(false, "a", false, pattern, value); }
static bool ends_with_b(const char* pattern, const char* value) { return ends_with(false, "b", true, pattern, value); }
static bool not_ends_with_a(const char* pattern, const char* value) { return ends_with(true, "a", false, pattern, value); }

static int test_ends_with()
{
    mu_assert(ends_with_a("", "abc"), "ends_with_in_nil");
    mu_assert(ends_with_a("abc", "aabc"), "ends_with_in_start");
    mu_assert(ends_with_a("abc", "abc"), "ends_with_in_exact");
    mu_assert(!ends_with_a("abc", "c"), "ends_with_out_var_smaller");
    mu_assert(!ends_with_a("abc", ""), "ends_with_out_var_nil");
    mu_assert(!ends_with_b("abc", "abc"), "ends_with_out_undef");
    // mu_assert(!not_ends_with_a("abc", "aabc"), "ends_with_not_ends);
    // mu_assert(!not_ends_with_a("abc", "abc"), "ends_with_not_exact");
    // mu_assert(not_ends_with_a("abc", "c"), "ends_with_not_var_smaller");
    // mu_assert(not_ends_with_a("abc", "", "ends_with_not_var_nil");
    return 0;
}

int all_tests() 
{
    // mu_run_test(test_within_frequency_cap);
    // mu_run_test(test_segment_within);
    // mu_run_test(test_segment_before);
    mu_run_test(test_geo);
    mu_run_test(test_contains);
    mu_run_test(test_starts_with);
    mu_run_test(test_ends_with);

    return 0;
}

RUN_TESTS()