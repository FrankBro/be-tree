#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "debug.h"
#include "minunit.h"
#include "tree.h"
#include "utils.h"

int parse(const char* text, struct ast_node** node);

struct value_bound get_bound(struct config* config, const char* expr)
{
    struct ast_node* node;
    parse(expr, &node);
    assign_variable_id(config, node);
    assign_str_id(config, node, false);
    struct value_bound bound = get_variable_bound(config->attr_domains[0], node);
    free_ast_node(node);
    return bound;
}

bool match_bool(struct config* config, const char* expr, bool min, bool max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_BOOLEAN && bound.bmin == min && bound.bmax == max;
    if(!result) {
        fprintf(stderr,
            "Min: Expected %d, Got %d. Max: Expected %d, Got %d\n",
            min,
            bound.bmin,
            max,
            bound.bmax);
    }
    return result;
}

bool match_integer(struct config* config, const char* expr, int64_t min, int64_t max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_INTEGER && bound.imin == min && bound.imax == max;
    if(!result) {
        fprintf(stderr,
            "Min: Expected %ld, Got %ld. Max: Expected %ld, Got %ld\n",
            min,
            bound.imin,
            max,
            bound.imax);
    }
    return result;
}

bool match_integer_list(struct config* config, const char* expr, int64_t min, int64_t max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_INTEGER_LIST && bound.imin == min && bound.imax == max;
    if(!result) {
        fprintf(stderr,
            "Min: Expected %ld, Got %ld. Max: Expected %ld, Got %ld\n",
            min,
            bound.imin,
            max,
            bound.imax);
    }
    return result;
}

bool match_float(struct config* config, const char* expr, double min, double max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_FLOAT && feq(bound.fmin, min) && feq(bound.fmax, max);
    if(!result) {
        fprintf(stderr,
            "Min: Expected %.0f, Got %.0f. Max: Expected %.0f, Got %.0f\n",
            min,
            bound.fmin,
            max,
            bound.fmax);
    }
    return result;
}

bool match_string(struct config* config, const char* expr, size_t min, size_t max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_STRING && bound.smin == min && bound.smax == max;
    if(!result) {
        fprintf(stderr,
            "Min: Expected %zu, Got %zu. Max: Expected %zu, Got %zu\n",
            min,
            bound.smin,
            max,
            bound.smax);
    }
    return result;
}

bool match_string_list(struct config* config, const char* expr, size_t min, size_t max)
{
    struct value_bound bound = get_bound(config, expr);
    bool result = bound.value_type == BETREE_STRING_LIST && bound.smin == min && bound.smax == max;
    if(!result) {
        fprintf(stderr,
            "Min: Expected %zu, Got %zu. Max: Expected %zu, Got %zu\n",
            min,
            bound.smin,
            max,
            bound.smax);
    }
    return result;
}

int test_bool_bounds()
{
    struct betree* tree = betree_make();
    add_attr_domain_b(tree->config, "b", true);
    add_attr_domain_b(tree->config, "wrong", true);

    mu_assert(match_bool(tree->config, "b", true, true), "single variable");
    mu_assert(match_bool(tree->config, "not b", false, false), "not variable");
    mu_assert(match_bool(tree->config, "b or not b", false, true), "variable or not variable");
    mu_assert(match_bool(tree->config, "b and not b", false, true), "variable and not variable");

    mu_assert(match_bool(tree->config, "wrong", false, true), "not in expression");

    betree_free(tree);

    return 0;
}

int test_integer_bounds()
{
    struct betree* tree = betree_make();
    int64_t min = -10;
    int64_t max = 10;
    add_attr_domain_bounded_i(tree->config, "i", false, min, max);
    add_attr_domain_b(tree->config, "wrong", true);

    mu_assert(match_integer(tree->config, "i = 1", 1, 1), "eq");
    mu_assert(match_integer(tree->config, "not (i = 1)", min, max), "not eq");
    mu_assert(match_integer(tree->config, "i <> 1", min, max), "ne");
    mu_assert(match_integer(tree->config, "not (i <> 1)", 1, 1), "not ne");

    mu_assert(match_integer(tree->config, "i > 1", 2, max), "gt");
    mu_assert(match_integer(tree->config, "not (i > 1)", min, 1), "not gt");
    mu_assert(match_integer(tree->config, "i >= 1", 1, max), "ge");
    mu_assert(match_integer(tree->config, "not (i >= 1)", min, 0), "not ge");
    mu_assert(match_integer(tree->config, "i < 1", min, 0), "lt");
    mu_assert(match_integer(tree->config, "not (i < 1)", 1, max), "not lt");
    mu_assert(match_integer(tree->config, "i <= 1", min, 1), "le");
    mu_assert(match_integer(tree->config, "not (i <= 1)", 2, max), "not le");

    mu_assert(match_integer(tree->config, "i in (2, 7)", 2, 7), "in");
    mu_assert(match_integer(tree->config, "not (i in (2, 7))", min, max), "not (in)");
    /*mu_assert(match_integer(tree->config, "i in ()", min, max), "empty in");*/
    /*mu_assert(match_integer(tree->config, "not (i in ())", min, max), "not (empty in)");*/
    mu_assert(match_integer(tree->config, "not (i not in (2, 7))", 2, 7), "not (not in)");
    /*mu_assert(match_integer(tree->config, "i not in ()", min, max), "empty not in");*/
    /*mu_assert(match_integer(tree->config, "not (i not in ())", min, max), "not (empty not in)");*/

    mu_assert(match_integer(tree->config, "wrong", min, max), "not in expression");

    betree_free(tree);

    return 0;
}

int test_float_bounds()
{
    struct betree* tree = betree_make();
    double min = -10.;
    double max = 10.;
    double change = __DBL_EPSILON__;
    add_attr_domain_bounded_f(tree->config, "f", false, min, max);
    add_attr_domain_b(tree->config, "wrong", true);

    mu_assert(match_float(tree->config, "f = 1.", 1., 1.), "eq");
    mu_assert(match_float(tree->config, "not (f = 1.)", min, max), "not eq");
    mu_assert(match_float(tree->config, "f <> 1.", min, max), "ne");
    mu_assert(match_float(tree->config, "not (f <> 1.)", 1., 1.), "not ne");

    mu_assert(match_float(tree->config, "f > 1.", 1. + change, max), "gt");
    mu_assert(match_float(tree->config, "not (f > 1.)", min, 1.), "not gt");
    mu_assert(match_float(tree->config, "f >= 1.", 1., max), "ge");
    mu_assert(match_float(tree->config, "not (f >= 1.)", min, 1. - change), "not ge");
    mu_assert(match_float(tree->config, "f < 1.", min, 1. - change), "lt");
    mu_assert(match_float(tree->config, "not (f < 1.)", 1., max), "not lt");
    mu_assert(match_float(tree->config, "f <= 1.", min, 1.), "le");
    mu_assert(match_float(tree->config, "not (f <= 1.)", 1. + change, max), "not le");

    mu_assert(match_float(tree->config, "wrong", min, max), "not in expression");

    betree_free(tree);

    return 0;
}

int test_string_bounds()
{
    struct betree* tree = betree_make();
    size_t min = 0;
    size_t max = 3;
    add_attr_domain_bounded_s(tree->config, "s", false, max + 1);
    add_attr_domain_b(tree->config, "wrong", true);

    // Add some useless exprs to make sure you pick all the str
    betree_insert(tree, 1, "s = \"a\"");
    betree_insert(tree, 2, "s = \"b\"");
    betree_insert(tree, 3, "s = \"c\"");
    betree_insert(tree, 4, "s = \"d\"");

    mu_assert(match_string(tree->config, "s = \"a\"", 0, 0), "eq");
    mu_assert(match_string(tree->config, "not (s = \"a\")", min, max), "not eq");
    mu_assert(match_string(tree->config, "s <> \"a\"", min, max), "ne");
    mu_assert(match_string(tree->config, "not (s <> \"a\")", 0, 0), "not ne");

    mu_assert(match_string(tree->config, "s in (\"b\", \"c\")", 1, 2), "in");
    mu_assert(match_string(tree->config, "not (s in (\"b\", \"c\"))", min, max), "not (in)");
    /*mu_assert(match_string(tree->config, "s in ()", min, max), "empty in");*/
    /*mu_assert(match_string(tree->config, "not (s in ())", min, max), "not (empty in)");*/
    mu_assert(match_string(tree->config, "s not in (\"b\", \"c\")", min, max), "not in");
    mu_assert(match_string(tree->config, "not (s not in (\"b\", \"c\"))", 1, 2), "not (not in)");
    /*mu_assert(match_string(tree->config, "s not in ()", min, max), "empty not in");*/
    /*mu_assert(match_string(tree->config, "not (s not in ())", min, max), "not (empty not in)");*/

    mu_assert(match_string(tree->config, "wrong", min, max), "not in expression");

    betree_free(tree);

    return 0;
}

int test_integer_list_bounds()
{
    struct betree* tree = betree_make();
    int64_t min = -10;
    int64_t max = 10;
    add_attr_domain_bounded_il(tree->config, "il", false, min, max);
    add_attr_domain_b(tree->config, "wrong", true);

    mu_assert(match_integer_list(tree->config, "1 in il", 1, 1), "in");
    mu_assert(match_integer_list(tree->config, "not (1 in il)", min, max), "not (in)");
    mu_assert(match_integer_list(tree->config, "1 not in il", min, max), "not in");
    mu_assert(match_integer_list(tree->config, "not (1 not in il)", 1, 1), "not (not in)");

    mu_assert(match_integer_list(tree->config, "il one of (2, 7)", 2, 7), "one of");
    mu_assert(match_integer_list(tree->config, "not (il one of (2, 7))", min, max), "not (one of)");
    /*mu_assert(match_integer_list(tree->config, "il one of ()", min, max), "empty one of");*/
    /*mu_assert(match_integer_list(tree->config, "not (il one of ())", min, max), "not (empty one
     * of)");*/
    mu_assert(match_integer_list(tree->config, "il all of (2, 7)", 2, 7), "all of");
    mu_assert(match_integer_list(tree->config, "not (il all of (2, 7))", min, max), "not (all of)");
    /*mu_assert(match_integer_list(tree->config, "il all of ()", min, max), "empty all of");*/
    /*mu_assert(match_integer_list(tree->config, "not (il all of ())", min, max), "not (empty all
     * of)");*/
    mu_assert(match_integer_list(tree->config, "il none of (2, 7)", min, max), "none of");
    mu_assert(match_integer_list(tree->config, "not (il none of (2, 7))", 2, 7), "not (none of)");
    /*mu_assert(match_integer_list(tree->config, "il none of ()", min, max), "empty none of");*/
    /*mu_assert(match_integer_list(tree->config, "not (il none of ())", min, max), "not (empty none
     * of)");*/

    mu_assert(match_integer_list(tree->config, "wrong", min, max), "not in expression");

    betree_free(tree);

    return 0;
}

int test_string_list_bounds()
{
    struct betree* tree
        = betree_make_with_parameters(5, 0); // to make sure it doesn't split for now
    size_t min = 0;
    size_t max = 3;
    add_attr_domain_bounded_sl(tree->config, "sl", false, max + 1);
    add_attr_domain_b(tree->config, "wrong", true);

    // Add some useless exprs to make sure you pick all the str
    betree_insert(tree, 1, "sl in (\"a\")");
    betree_insert(tree, 2, "sl in (\"b\")");
    betree_insert(tree, 3, "sl in (\"c\")");
    betree_insert(tree, 4, "sl in (\"d\")");

    mu_assert(match_string_list(tree->config, "\"b\" in sl", 1, 1), "in");
    mu_assert(match_string_list(tree->config, "not (\"b\" in sl)", min, max), "not (in)");
    mu_assert(match_string_list(tree->config, "\"b\" not in sl", min, max), "not in");
    mu_assert(match_string_list(tree->config, "not (\"b\" not in sl)", 1, 1), "not (not in)`");

    mu_assert(match_string_list(tree->config, "sl one of (\"b\", \"c\")", 1, 2), "one of");
    mu_assert(match_string_list(tree->config, "not (sl one of (\"b\", \"c\"))", min, max),
        "not (one of)");
    /*mu_assert(match_string_list(tree->config, "sl one of ()", min, max), "empty one of");*/
    /*mu_assert(match_string_list(tree->config, "not (sl one of ())", min, max), "not (empty one
     * of)");*/
    mu_assert(match_string_list(tree->config, "sl all of (\"b\", \"c\")", 1, 2), "all of");
    mu_assert(match_string_list(tree->config, "not (sl all of (\"b\", \"c\"))", min, max),
        "not (all of)");
    /*mu_assert(match_string_list(tree->config, "sl all of ()", min, max), "empty all of");*/
    /*mu_assert(match_string_list(tree->config, "not (sl all of ())", min, max), "not (empty all
     * of)");*/
    mu_assert(match_string_list(tree->config, "sl none of (\"b\", \"c\")", min, max), "none of");
    mu_assert(
        match_string_list(tree->config, "not (sl none of (\"b\", \"c\"))", 1, 2), "not (none of)");
    /*mu_assert(match_string_list(tree->config, "sl none of ()", min, max), "empty none of");*/
    /*mu_assert(match_string_list(tree->config, "not (sl none of ())", min, max), "not (empty none
     * of)");*/

    mu_assert(match_string_list(tree->config, "wrong", min, max), "not in expression");

    betree_free(tree);

    return 0;
}

int test_complex_bounds()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "i", false, 0, 10);
    add_attr_domain_b(tree->config, "b", true);

    mu_assert(match_integer(tree->config, "i > 5 and i < 8", 6, 7), "and");
    mu_assert(match_integer(tree->config, "i > 5 or i > 6", 6, 10), "or");
    mu_assert(match_integer(tree->config, "(i > 5 and i < 8) or (i > 6)", 6, 10), "and or");

    mu_assert(match_integer(tree->config, "i > 5 and i > 8", 6, 10), "and");
    mu_assert(match_integer(tree->config, "not (i > 5 and i > 8)", 0, 8), "not and");
    mu_assert(match_integer(tree->config, "i > 5 or i > 6", 6, 10), "or");
    mu_assert(match_integer(tree->config, "not (i > 5 or i > 6)", 0, 6), "not or");

    mu_assert(match_integer(tree->config, "i > 5 and b", 6, 10), "and l");
    mu_assert(match_integer(tree->config, "i > 5 or b", 0, 10), "or l");

    return 0;
}

int all_tests()
{
    mu_run_test(test_bool_bounds);
    mu_run_test(test_integer_bounds);
    mu_run_test(test_float_bounds);
    mu_run_test(test_string_bounds);
    mu_run_test(test_integer_list_bounds);
    mu_run_test(test_string_list_bounds);
    mu_run_test(test_complex_bounds);

    return 0;
}

RUN_TESTS()
