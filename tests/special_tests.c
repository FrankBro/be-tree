#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"
#include "utils.h"

int parse(const char *text, struct ast_node **node);

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

int all_tests() 
{
    // mu_run_test(test_within_frequency_cap);
    // mu_run_test(test_segment_within);
    // mu_run_test(test_segment_before);
    // mu_run_test(test_geo);
    mu_run_test(test_contains);
    mu_run_test(test_starts_with);
    // mu_run_test(test_ends_with);

    return 0;
}

RUN_TESTS()