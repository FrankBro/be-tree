#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "parser.h"
#include "minunit.h"
#include "tree.h"
#include "utils.h"

int parse(const char *text, struct ast_node **node);

static bool 
frequency(
    bool has_not,
    enum frequency_type_e type, const char* ns, int64_t value, int64_t length, 
    int64_t now, 
    enum frequency_type_e cap_type, uint32_t cap_id, const char* cap_ns, uint32_t cap_value, int64_t timestamp)
{
    enum e { constant_count = 4 };
    const struct betree_constant* constants[constant_count] = {
        betree_make_integer_constant("flight_id", 10),
        betree_make_integer_constant("advertiser_id", 20),
        betree_make_integer_constant("campaign_id", 30),
        betree_make_integer_constant("product_id", 40),
    };
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "now", false, 0, 10);
    const char* frequency_attr = "frequency_caps";
    add_attr_domain_frequency(tree->config, frequency_attr, false);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    int64_t usec = 1000 * 1000;
    const char* type_value = frequency_type_to_string(type);
    if(basprintf(&expr, "%swithin_frequency_cap(\"%s\", \"%s\", %" PRId64 ", %" PRId64 ")", pre, type_value, ns, value, length) < 0) {
        abort();
    }
    betree_insert_with_constants(tree, 1, constant_count, constants, expr);
    char* event_str;
    const char* cap_type_value = frequency_type_to_string(cap_type);
    if(basprintf(&event_str, "{\"now\": %ld, \"frequency_caps\": [[\"%s\", %u, \"%s\", %d, %ld]]}", now, cap_type_value, cap_id, cap_ns, cap_value, timestamp * usec) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    for(size_t i = 0; i < constant_count; i++) {
        betree_free_constant((struct betree_constant*)constants[i]);
    }
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
    return result;
}

static bool within_frequency_cap(
    enum frequency_type_e type, const char* ns, int64_t value, int64_t length, 
    int64_t now, 
    enum frequency_type_e cap_type, uint32_t cap_id, const char* cap_ns, uint32_t cap_value, int64_t timestamp) 
    { return frequency(false, type, ns, value, length, now, cap_type, cap_id, cap_ns, cap_value, timestamp); }
static bool not_within_frequency_cap(
    enum frequency_type_e type, const char* ns, int64_t value, int64_t length, 
    int64_t now, 
    enum frequency_type_e cap_type, uint32_t cap_id, const char* cap_ns, uint32_t cap_value, int64_t timestamp) 
    { return frequency(true, type, ns, value, length, now, cap_type, cap_id, cap_ns, cap_value, timestamp); }

int test_frequency()
{
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 0, 0
    ), "fcap_type_flight");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHTIP, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHTIP, 10, "ns", 0, 0
    ), "fcap_type_flight_ip");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_ADVERTISER, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_ADVERTISER, 20, "ns", 0, 0
    ), "fcap_type_advertiser");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_ADVERTISERIP, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_ADVERTISERIP, 20, "ns", 0, 0
    ), "fcap_type_advertiser_ip");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_CAMPAIGN, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_CAMPAIGN, 30, "ns", 0, 0
    ), "fcap_type_campaign");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_CAMPAIGNIP, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_CAMPAIGNIP, 30, "ns", 0, 0
    ), "fcap_type_campaign_ip");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_PRODUCT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_PRODUCT, 40, "ns", 0, 0
    ), "fcap_type_product");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_PRODUCTIP, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_PRODUCTIP, 40, "ns", 0, 0
    ), "fcap_type_product_ip");

    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_PRODUCT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 200, 0
    ), "fcap_id_type_ne");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 1, "ns", 200, 0
    ), "fcap_id_id_ne");
    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "blah", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 200, 0
    ), "fcap_id_ns_ne");

    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 99, 0
    ), "fcap_val_lt");
    mu_assert(!within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 100, 0
    ), "fcap_val_eq");
    mu_assert(!within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 0,
        0, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 101, 0
    ), "fcap_val_gt");

    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 20,
        40, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 200, 10
    ), "fcap_ts_lt");
    mu_assert(!within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 20,
        40, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 200, 20
    ), "fcap_ts_eq");
    mu_assert(!within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 20,
        40, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 200, 30
    ), "fcap_ts_gt");

    mu_assert(within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 20,
        40, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 99, 30
    ), "fcap_val_lt_ts_gt");

    mu_assert(!not_within_frequency_cap(
        FREQUENCY_TYPE_FLIGHT, "ns", 100, 20,
        40, 
        FREQUENCY_TYPE_FLIGHT, 10, "ns", 99, 30
    ), "fcap_not_val_lt_ts_gt");
    return 0;
}

enum segment_function_type {
    SEGMENT_WITHIN,
    SEGMENT_BEFORE,
};

enum segment_var_type {
    NO_VAR,
    FIRST_VAR,
    SECOND_VAR,
};

static bool 
segment(bool has_not, enum segment_function_type func_type, 
    enum segment_var_type var_type, int64_t id, int64_t seconds, 
    int64_t segment_id, int64_t segment_seconds)
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "now", false, 0.0, 10.0);
    add_attr_domain_segments(tree->config, "seg_a", false);
    add_attr_domain_segments(tree->config, "seg_b", false);
    add_attr_domain_segments(tree->config, "segments_with_timestamp", false);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    const char* func;
    switch(func_type) {
        case SEGMENT_WITHIN:
            func = "segment_within";
            break;
        case SEGMENT_BEFORE:
            func = "segment_before";
            break;
        default:
            abort();
            break;
    }
    const char* var;
    switch(var_type) {
        case NO_VAR:
            var = "";
            break;
        case FIRST_VAR: 
            var = "seg_a, ";
            break;
        case SECOND_VAR: 
            var = "seg_b, ";
            break;
        default:
            abort();
            break;
    }
    int64_t usec = 1000 * 1000;
    if(basprintf(&expr, "%s%s(%s%" PRId64 ", %" PRId64 ")", pre, func, var, id, seconds) < 0) {
        abort();
    }
    betree_insert(tree, 1, expr);
    char* event_str;
    if(basprintf(&event_str, "{\"now\": 40, \"seg_a\": [[1, %ld]], \"seg_b\": [[1, %ld]], \"segments_with_timestamp\": [[%ld, %ld]]}", 30 * usec, 10 * usec, segment_id, segment_seconds * usec) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
    return result;
}

static bool segment_within(int64_t id, int64_t seconds, int64_t segment_id, int64_t segment_seconds) { return segment(false, SEGMENT_WITHIN, NO_VAR, id, seconds, segment_id, segment_seconds); }
static bool segment_within_a(int64_t id, int64_t seconds) { return segment(false, SEGMENT_WITHIN, FIRST_VAR, id, seconds, 0, 0); }
static bool segment_within_b(int64_t id, int64_t seconds) { return segment(false, SEGMENT_WITHIN, SECOND_VAR, id, seconds, 0, 0); }

static bool segment_before(int64_t id, int64_t seconds, int64_t segment_id, int64_t segment_seconds) { return segment(false, SEGMENT_BEFORE, NO_VAR, id, seconds, segment_id, segment_seconds); }
static bool segment_before_a(int64_t id, int64_t seconds) { return segment(false, SEGMENT_BEFORE, FIRST_VAR, id, seconds, 0, 0); }
static bool segment_before_b(int64_t id, int64_t seconds) { return segment(false, SEGMENT_BEFORE, SECOND_VAR, id, seconds, 0, 0); }

static bool not_segment_within(int64_t id, int64_t seconds, int64_t segment_id, int64_t segment_seconds) { return segment(true, SEGMENT_WITHIN, NO_VAR, id, seconds, segment_id, segment_seconds); }
static bool not_segment_before(int64_t id, int64_t seconds, int64_t segment_id, int64_t segment_seconds) { return segment(true, SEGMENT_BEFORE, NO_VAR, id, seconds, segment_id, segment_seconds); }

int test_segment()
{
    mu_assert(segment_within(1, 20, 1, 30), "segment_within_id_eq");
    mu_assert(!segment_within(1, 20, 2, 30), "segment_within_id_ne");
    mu_assert(!segment_within(1, 20, 1, 10), "segment_within_ts_lt");
    mu_assert(segment_within(1, 20, 1, 20), "segment_within_ts_eq");
    mu_assert(segment_within(1, 20, 1, 30), "segment_within_ts_gt");
    mu_assert(segment_before(1, 20, 1, 10), "segment_before_id_eq");
    mu_assert(!segment_before(1, 20, 2, 10), "segment_before_id_ne");
    mu_assert(segment_before(1, 20, 1, 10), "segment_before_ts_lt");
    mu_assert(!segment_before(1, 20, 1, 20), "segment_before_ts_eq");
    mu_assert(!segment_before(1, 20, 1, 30), "segment_before_ts_gt");

    mu_assert(not_segment_before(1, 20, 1, 30), "segment_not_before_ts_gt");
    mu_assert(!not_segment_within(1, 20, 1, 30), "segment_not_within_ts_gt");

    mu_assert(segment_before(200030624864, 20, 200030624864, 10), "segment_before_big_id_in");
    mu_assert(!segment_before(200030624864, 20, 1, 10), "segment_before_big_id_out");
    mu_assert(segment_within(200030624864, 20, 200030624864, 30), "segment_within_big_id_in");
    mu_assert(!segment_within(200030624864, 20, 1, 30), "segment_within_big_id_out");

    mu_assert(segment_within_a(1, 20), "segvar_within_a");
    mu_assert(!segment_within_b(1, 20), "segvar_within_b");

    mu_assert(!segment_before_a(1, 20), "segvar_before_a");
    mu_assert(segment_before_b(1, 20), "segvar_before_b");
    return 0;
}

static bool geo(bool has_not, const char* latitude, const char* longitude, const char* radius, double latitude_value, double longitude_value)
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_f(tree->config, "latitude", false, 0.0, 10.0);
    add_attr_domain_bounded_f(tree->config, "longitude", false, 0.0, 10.0);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    if(basprintf(&expr, "%sgeo_within_radius(%s, %s, %s)", pre, latitude, longitude, radius) < 0) {
        abort();
    }
    betree_insert(tree, 1, expr);
    char* event_str;
    if(basprintf(&event_str, "{\"latitude\": %.1f, \"longitude\": %.1f}", latitude_value, longitude_value) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
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
    mu_assert(!not_geo_within_radius("100.0", "100.0", "10.0", 100.0, 100.0), "geo_not_within_float_inside");
    mu_assert(not_geo_within_radius("100.0", "100.0", "10.0", 200.0, 200.0), "geo_not_within_float_outside");
    return 0;
}

static bool contains(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", true);
    add_attr_domain_s(tree->config, "b", true);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    if(basprintf(&expr, "%scontains(%s, \"%s\")", pre, attr, pattern) < 0) {
        abort();
    }
    betree_insert(tree, 1, expr);
    const char* event_attr = allow_undefined ? "a" : attr;
    char* event_str;
    if(basprintf(&event_str, "{\"%s\": \"%s\"}", event_attr, value) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
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
    mu_assert(!not_contains_a("abc", "aabcc"), "contains_not_mid");
    mu_assert(!not_contains_a("abc", "abc"), "contains_not_exact");
    mu_assert(not_contains_a("abc", "aabbcc"), "contains_not_var_bigger");
    mu_assert(not_contains_a("z", "abc"), "contains_not_pattern_smaller");
    return 0;
}

static bool starts_with(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", true);
    add_attr_domain_s(tree->config, "b", true);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    if(basprintf(&expr, "%sstarts_with(%s, \"%s\")", pre, attr, pattern) < 0) {
        abort();
    }
    betree_insert(tree, 1, expr);
    const char* event_attr = allow_undefined ? "a" : attr;
    char* event_str;
    if(basprintf(&event_str, "{\"%s\": \"%s\"}", event_attr, value) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
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
    mu_assert(!not_starts_with_a("abc", "abcc"), "starts_with_not_start");
    mu_assert(!not_starts_with_a("abc", "abc"), "starts_with_not_exact");
    mu_assert(not_starts_with_a("abc", "a"), "starts_with_not_var_smaller");
    mu_assert(not_starts_with_a("abc", ""), "starts_with_not_var_nil");
    return 0;
}

static bool ends_with(bool has_not, const char* attr, bool allow_undefined, const char* pattern, const char* value)
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", true);
    add_attr_domain_s(tree->config, "b", true);
    char* expr;
    const char* pre;
    if(has_not) {
        pre = "not ";
    }
    else {
        pre = "";
    }
    if(basprintf(&expr, "%sends_with(%s, \"%s\")", pre, attr, pattern) < 0) {
        abort();
    }
    betree_insert(tree, 1, expr);
    const char* event_attr = allow_undefined ? "a" : attr;
    char* event_str;
    if(basprintf(&event_str, "{\"%s\": \"%s\"}", event_attr, value) < 0) {
        abort();
    }
    struct report* report = make_report();
    if(betree_search(tree, event_str, report) == false) {
        fprintf(stderr, "Failed to search for event\n");
        abort();
    }
    bool result = report->matched == 1;
    free(expr);
    free(event_str);
    free_report(report);
    betree_free(tree);
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
    mu_assert(!not_ends_with_a("abc", "aabc"), "ends_with_not_ends");
    mu_assert(!not_ends_with_a("abc", "abc"), "ends_with_not_exact");
    mu_assert(not_ends_with_a("abc", "c"), "ends_with_not_var_smaller");
    mu_assert(not_ends_with_a("abc", ""), "ends_with_not_var_nil");
    return 0;
}

int all_tests() 
{
    mu_run_test(test_frequency);
    mu_run_test(test_segment);
    mu_run_test(test_geo);
    mu_run_test(test_contains);
    mu_run_test(test_starts_with);
    mu_run_test(test_ends_with);

    return 0;
}

RUN_TESTS()
