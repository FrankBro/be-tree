
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

// void add_attr_domain_i(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_f(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_b(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined);
// void add_attr_domain_ie(struct config* config, const char* attr, bool allow_undefined);

// 1, 1.1, "1", (1, 2), ("1", "2"), true

enum valid_domain_e {
    VALID_DOMAIN_I,
    VALID_DOMAIN_F,
    VALID_DOMAIN_B,
    VALID_DOMAIN_S,
    VALID_DOMAIN_IL,
    VALID_DOMAIN_SL,
    VALID_DOMAIN_SEGMENTS,
    VALID_DOMAIN_FREQUENCY,
    VALID_DOMAIN_IE,
};

const char* valid_domain_e_name(enum valid_domain_e domain)
{
    switch(domain) {
        case VALID_DOMAIN_I: return "integer";
        case VALID_DOMAIN_F: return "float";
        case VALID_DOMAIN_B: return "boolean";
        case VALID_DOMAIN_S: return "string";
        case VALID_DOMAIN_IL: return "integer list";
        case VALID_DOMAIN_SL: return "string list";
        case VALID_DOMAIN_SEGMENTS: return "segments";
        case VALID_DOMAIN_FREQUENCY: return "frequency";
        case VALID_DOMAIN_IE: return "integer enum";
        default: return "INVALID";
    }
}

int test_valid(enum valid_domain_e domain, bool allow_undefined, const char* expr, bool is_valid, const char* event, size_t matches)
{
    struct betree* tree = betree_make();
    switch(domain) {
        case VALID_DOMAIN_I:
            add_attr_domain_i(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_F:
            add_attr_domain_f(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_B:
            add_attr_domain_b(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_S:
            add_attr_domain_s(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_IL:
            add_attr_domain_il(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_SL:
            add_attr_domain_sl(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_SEGMENTS:
            add_attr_domain_segments(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_FREQUENCY:
            add_attr_domain_frequency(tree->config, "var", allow_undefined);
            break;
        case VALID_DOMAIN_IE:
            add_attr_domain_ie(tree->config, "var", allow_undefined);
            break;
        default:
            mu_assert(false, "Invalid valid domain enum");
    }
    const struct betree_sub* sub = betree_make_sub(tree, 0, 0, NULL, expr);
    if(is_valid) {
        mu_assert(sub != NULL, "");
        betree_insert_sub(tree, sub);
        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");
        mu_assert(report->matched == matches, "domain: %s, expr: %s, event: %s", valid_domain_e_name(domain), expr, event);
        free_report(report);
    }
    else {
        mu_assert(sub == NULL, "");
    }
    betree_free(tree);
    return 0;
}

int test_missing_domain()
{
    struct betree* tree = betree_make();
    const struct betree_sub* sub = NULL;
    sub = betree_make_sub(tree, 0, 0, NULL, "var = 1");
    mu_assert(sub == NULL, "");
    sub = betree_make_sub(tree, 0, 0, NULL, "var = \"1\"");
    mu_assert(sub == NULL, "");
    betree_free(tree);
    return 0;
}

int test_comparison()
{
    // For the event, we don't need to test integer values because in the real code, this never happens
    // Integer
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var < 1", true, "{\"var\": 2}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var < 1", true, "{\"var\": 0}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var < 1", true, "{\"var\": 2.0}", 0) == 0, ""); // fix_float_with_no_fractions fixes this
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var < 1", true, "{\"var\": 0.5}", 1) == 0, ""); // fix_float_with_no_fractions fixes this
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var < 1", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var < 1", false, "", 0) == 0, "");
    // Float
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var < 1.2", true, "{\"var\": 2.4}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var < 1.2", true, "{\"var\": 0.9}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var < 1.2", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var < 1.2", false, "", 0) == 0, "");
    return 0;
}

int test_equality()
{
    // For the event, we don't need to test integer values because in the real code, this never happens
    // Integer and integer enum
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var = 0", true, "{\"var\": 2}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var = 0", true, "{\"var\": 2.0}", 0) == 0, ""); // fix_float_with_no_fractions fixes this
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var = 0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var = 0", true, "{\"var\": 450}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var = 450", true, "{\"var\": 450}", 1) == 0, ""); 
    // Float
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var = 0.0", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var = 0.0", true, "{\"var\": 2.0}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var = 0.0", true, "{\"var\": 0.0}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var = 0.0", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var = 0.0", false, "", 0) == 0, ""); 
    // String
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var = \"value\"", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var = \"value\"", true, "{\"var\": \"wrong\"}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var = \"value\"", true, "{\"var\": \"value\"}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var = \"value\"", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var = \"value\"", false, "", 0) == 0, ""); 
    return 0;
}

int test_boolean()
{
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var", true, "{\"var\": false}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var", true, "{\"var\": true}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var", false, "", 0) == 0, ""); 
    return 0;
}

int test_set()
{
    // Left value, integer
    mu_assert(test_valid(VALID_DOMAIN_I, true, "1 in var", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "1 in var", true, "{\"var\": [0,2]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "1 in var", true, "{\"var\": [0,1,2]}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "1 in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "1 in var", false, "", 0) == 0, ""); 
    // Left value, string
    mu_assert(test_valid(VALID_DOMAIN_I, true, "\"value\" in var", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "\"value\" in var", true, "{\"var\": [\"wrong\",\"another\"]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "\"value\" in var", true, "{\"var\": [\"wrong\",\"value\",\"another\"]}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "\"value\" in var", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "\"value\" in var", false, "", 0) == 0, ""); 
    // Right value, integer list
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var in (0, 2)", true, "{\"var\": 1}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var in (0, 2)", true, "{\"var\": 2}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var in (0, 2)", false, "", 0) == 0, ""); 
    // Right value, string list
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var in (\"value\", \"another\")", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var in (\"value\", \"another\")", true, "{\"var\": \"wrong\"}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var in (\"value\", \"another\")", true, "{\"var\": \"value\"}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var in (\"value\", \"another\")", false, "", 0) == 0, ""); 
    return 0;
}

int test_list()
{
    // Integer list
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var one of (0, 2)", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var one of (0, 2)", true, "{\"var\": [1, 3]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var one of (0, 2)", true, "{\"var\": [1, 2]}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var one of (0, 2)", false, "", 0) == 0, ""); 
    // String list
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var one of (\"value\", \"nope\")", true, "{\"var\": [\"wrong\", \"no\"]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var one of (\"value\", \"nope\")", true, "{\"var\": [\"wrong\", \"value\"]}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var one of (\"value\", \"nope\")", false, "", 0) == 0, ""); 
    return 0;
}

int test_null()
{
    // null
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var is null", true, "{\"var\": 1}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_I, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var is null", true, "{\"var\": 1.0}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var is null", true, "{\"var\": true}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_B, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var is null", true, "{\"var\": \"value\"}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_S, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var is null", true, "{\"var\": [1,2]}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IL, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var is null", true, "{\"var\": [\"value\", \"another\"]}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_SL, false, "var is null", false, "", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var is null", true, "{\"var\": 2}", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var is null", true, "{}", 1) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_IE, false, "var is null", false, "", 1) == 0, "");
    // empty
    mu_assert(test_valid(VALID_DOMAIN_I, true, "var is empty", false, "", 0) == 0, "");
    mu_assert(test_valid(VALID_DOMAIN_F, true, "var is empty", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_B, true, "var is empty", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_S, true, "var is empty", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var is empty", true, "{\"var\": [1,2]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IL, true, "var is empty", true, "{\"var\": []}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var is empty", true, "{\"var\": [\"value\", \"another\"]}", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SL, true, "var is empty", true, "{\"var\": []}", 1) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_SEGMENTS, true, "var is empty", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_FREQUENCY, true, "var is empty", false, "", 0) == 0, ""); 
    mu_assert(test_valid(VALID_DOMAIN_IE, true, "var is empty", false, "", 0) == 0, ""); 
    return 0;
}

int test_special()
{
    // frequency
    enum e { constant_count = 5 };
    const struct betree_constant* constants[constant_count] = {
        betree_make_integer_constant("flight_id", 10),
        betree_make_integer_constant("advertiser_id", 20),
        betree_make_integer_constant("campaign_id", 30),
        betree_make_integer_constant("product_id", 40),
        betree_make_integer_constant("unknown", 50),
    };
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "now", false);
    add_attr_domain_frequency(tree->config, "frequency_caps", false);
    const char* expr = "within_frequency_cap(\"flight\", \"ns\", 100, 0)";
    const struct betree_sub* sub = betree_make_sub(tree, 0, constant_count, constants, expr);
    mu_assert(sub != NULL, "");
    expr = "within_frequency_cap(\"invalid\", \"ns\", 100, 0)";
    sub = betree_make_sub(tree, 1, constant_count, constants, expr);
    mu_assert(sub == NULL, "");
    // search
    struct betree_frequency_cap* frequency_cap = betree_make_frequency_cap("invalid", 0, "ns", false, 0, 0);
    mu_assert(sub == NULL, "");
    betree_free(tree);
    return 0;
}

int all_tests()
{
    test_missing_domain();
    test_comparison();
    test_equality();
    test_boolean();
    test_set();
    test_list();
    test_null();
    test_special();
    return 0;
}

RUN_TESTS()

