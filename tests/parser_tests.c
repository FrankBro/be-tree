#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "minunit.h"
#include "parser.h"
#include "utils.h"

int parse(const char* text, struct ast_node** node);

int test_is_null()
{
    struct ast_node* node = NULL;
    parse("a is null", &node);
    mu_assert(node->type == AST_TYPE_IS_NULL_EXPR && node->is_null_expr.type == AST_IS_NULL && strcmp(node->is_null_expr.attr_var.attr, "a") == 0, "is null");
    free_ast_node(node);
    parse("a is not null", &node);
    mu_assert(node->type == AST_TYPE_IS_NULL_EXPR && node->is_null_expr.type == AST_IS_NOT_NULL && strcmp(node->is_null_expr.attr_var.attr, "a") == 0, "is not null");
    free_ast_node(node);
    return 0;
}

int test_all_compare()
{
    struct ast_node* node = NULL;
    parse("a < 0", &node);
    mu_assert(node->type == AST_TYPE_COMPARE_EXPR && node->compare_expr.op == AST_COMPARE_LT, "LT");
    free_ast_node(node);
    parse("a <= 0", &node);
    mu_assert(node->type == AST_TYPE_COMPARE_EXPR && node->compare_expr.op == AST_COMPARE_LE, "LE");
    free_ast_node(node);
    parse("a > 0", &node);
    mu_assert(node->type == AST_TYPE_COMPARE_EXPR && node->compare_expr.op == AST_COMPARE_GT, "GT");
    free_ast_node(node);
    parse("a >= 0", &node);
    mu_assert(node->type == AST_TYPE_COMPARE_EXPR && node->compare_expr.op == AST_COMPARE_GE, "GE");
    free_ast_node(node);
    parse("a > -1", &node);
    mu_assert(node->type == AST_TYPE_COMPARE_EXPR && node->compare_expr.op == AST_COMPARE_GT
            && node->compare_expr.value.value_type == AST_COMPARE_VALUE_INTEGER
            && node->compare_expr.value.integer_value == -1,
        "minus");
    free_ast_node(node);
    return 0;
}

int test_all_equality()
{
    struct ast_node* node = NULL;
    parse("a = 0", &node);
    mu_assert(
        node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ, "EQ");
    free_ast_node(node);
    parse("a <> 0", &node);
    mu_assert(
        node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_NE, "NE");
    free_ast_node(node);
    parse("a = -1", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_INTEGER
            && node->equality_expr.value.integer_value == -1,
        "minus");
    free_ast_node(node);
    return 0;
}

int test_paren()
{
    struct ast_node* node = NULL;
    parse("(a = 0 && b = 0) || c = 0", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR, "first");
    free_ast_node(node);
    parse("a = 0 && (b = 0 || c = 0)", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND, "second");
    free_ast_node(node);
    return 0;
}

int test_precedence()
{
    struct ast_node* a = NULL;
    struct ast_node* b = NULL;

    parse("x and y and z", &a);
    parse("(x and y) and z", &b);
    mu_assert(eq_expr(a, b), "'and' is left-associative");
    free_ast_node(a);
    free_ast_node(b);

    parse("x or y or z", &a);
    parse("(x or y) or z", &b);
    mu_assert(eq_expr(a, b), "'or' is left-associative");
    free_ast_node(a);
    free_ast_node(b);

    parse("x or y and z", &a);
    parse("x or (y and z)", &b);
    mu_assert(eq_expr(a, b), "'and' has higher precedence than 'or'");
    free_ast_node(a);
    free_ast_node(b);

    parse("(x or y) and z", &a);
    parse("x or y and z", &b);
    mu_assert(!eq_expr(a, b), "parentheses can be used to force higher precedence");
    free_ast_node(a);
    free_ast_node(b);

    parse("not x or y and z", &a);
    parse("(not x) or y and z", &b);
    mu_assert(eq_expr(a, b), "'not' has the highest precedence");
    free_ast_node(a);
    free_ast_node(b);

    return 0;
}

int test_float()
{
    struct ast_node* node = NULL;
    parse("a = 0.", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_FLOAT
            && feq(node->equality_expr.value.float_value, 0.),
        "no decimal");
    free_ast_node(node);
    parse("a = 0.0", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_FLOAT
            && feq(node->equality_expr.value.float_value, 0.),
        "with decimal");
    free_ast_node(node);
    parse("a = - 1.", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_FLOAT
            && feq(node->equality_expr.value.float_value, -1.),
        "minus");
    free_ast_node(node);
    return 0;
}

int test_bool()
{
    struct ast_node* node = NULL;
    parse("a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_VARIABLE, "none");
    free_ast_node(node);
    parse("not a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_NOT, "not");
    free_ast_node(node);
    parse("a = 0 && b = 0", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND, "AND");
    free_ast_node(node);
    parse("a = 0 || b = 0", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR, "OR");
    free_ast_node(node);
    parse("a = 0 and b = 0", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND, "AND");
    free_ast_node(node);
    parse("a = 0 or b = 0", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR, "OR");
    free_ast_node(node);
    return 0;
}

int test_string()
{
    struct ast_node* node = NULL;
    parse("a = \"a\"", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING
            && strcmp(node->equality_expr.value.string_value.string, "a") == 0
            && strcmp(node->equality_expr.attr_var.attr, "a") == 0,
        "eq");
    free_ast_node(node);
    parse("a <> \"a\"", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_NE
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING
            && strcmp(node->equality_expr.value.string_value.string, "a") == 0
            && strcmp(node->equality_expr.attr_var.attr, "a") == 0,
        "ne");
    free_ast_node(node);
    parse("a = 'a'", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING
            && strcmp(node->equality_expr.value.string_value.string, "a") == 0
            && strcmp(node->equality_expr.attr_var.attr, "a") == 0,
        "single quote");
    free_ast_node(node);
    parse("a = 'a.b'", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ
            && node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING
            && strcmp(node->equality_expr.value.string_value.string, "a.b") == 0
            && strcmp(node->equality_expr.attr_var.attr, "a") == 0,
        "dot");
    free_ast_node(node);
    return 0;
}

int test_integer_set()
{
    struct ast_node* node = NULL;
    parse("a in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST,
        "in");
    free_ast_node(node);
    parse("a not in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_NOT_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST,
        "in");
    free_ast_node(node);
    parse("a in (1)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST,
        "single");
    free_ast_node(node);
    parse("1 in a", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_INTEGER
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE,
        "flipped");
    free_ast_node(node);
    parse("a in (-1)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST
            && node->set_expr.right_value.integer_list_value->integers[0] == -1,
        "minus");
    free_ast_node(node);
    parse("-1 in a", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_INTEGER
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE
            && node->set_expr.left_value.integer_value == -1,
        "minus flipped");
    free_ast_node(node);
    return 0;
}

int test_string_set()
{
    struct ast_node* node = NULL;
    parse("a in (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST,
        "in");
    free_ast_node(node);
    parse("a not in (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_NOT_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST,
        "in");
    free_ast_node(node);
    parse("a in (\"1\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST,
        "single");
    free_ast_node(node);
    parse("\"1\" in a", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR && node->set_expr.op == AST_SET_IN
            && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING
            && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE,
        "flipped");
    free_ast_node(node);
    return 0;
}

int test_integer_list()
{
    struct ast_node* node = NULL;
    parse("a one of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_ONE_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST,
        "one of");
    free_ast_node(node);
    parse("a none of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_NONE_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST,
        "none of");
    free_ast_node(node);
    parse("a all of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_ALL_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST,
        "all of");
    free_ast_node(node);
    parse("a one of (-1)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_ONE_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST
            && node->list_expr.value.integer_list_value->integers[0] == -1,
        "minus");
    free_ast_node(node);
    return 0;
}

int test_string_list()
{
    struct ast_node* node = NULL;
    parse("a one of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_ONE_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST,
        "one of");
    free_ast_node(node);
    parse("a none of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_NONE_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST,
        "none of");
    free_ast_node(node);
    parse("a all of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR && node->list_expr.op == AST_LIST_ALL_OF
            && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST,
        "all of");
    free_ast_node(node);
    return 0;
}

int test_special()
{
    struct ast_node* node = NULL;
    parse("within_frequency_cap(\"campaign\", \"namespace\", 100, 0)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR
            && node->special_expr.type == AST_SPECIAL_FREQUENCY
            && node->special_expr.frequency.op == AST_SPECIAL_WITHINFREQUENCYCAP
            && node->special_expr.frequency.type == FREQUENCY_TYPE_CAMPAIGN
            && strcmp(node->special_expr.frequency.ns.string, "namespace") == 0
            && node->special_expr.frequency.value == 100
            && node->special_expr.frequency.length == 0,
        "within_frequency_cap");
    free_ast_node(node);
    parse("segment_within(1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_SEGMENT
            && node->special_expr.segment.op == AST_SPECIAL_SEGMENTWITHIN
            && node->special_expr.segment.has_variable == false
            && node->special_expr.segment.segment_id == 1
            && node->special_expr.segment.seconds == 20,
        "segment_within");
    free_ast_node(node);
    parse("segment_before(1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_SEGMENT
            && node->special_expr.segment.op == AST_SPECIAL_SEGMENTBEFORE
            && node->special_expr.segment.has_variable == false
            && node->special_expr.segment.segment_id == 1
            && node->special_expr.segment.seconds == 20,
        "segment_before");
    free_ast_node(node);
    parse("segment_within(name, 1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_SEGMENT
            && node->special_expr.segment.op == AST_SPECIAL_SEGMENTWITHIN
            && node->special_expr.segment.has_variable == true
            && strcmp(node->special_expr.segment.attr_var.attr, "name") == 0
            && node->special_expr.segment.segment_id == 1
            && node->special_expr.segment.seconds == 20,
        "segment_within");
    free_ast_node(node);
    parse("segment_before(name, 1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_SEGMENT
            && node->special_expr.segment.op == AST_SPECIAL_SEGMENTBEFORE
            && node->special_expr.segment.has_variable == true
            && strcmp(node->special_expr.segment.attr_var.attr, "name") == 0
            && node->special_expr.segment.segment_id == 1
            && node->special_expr.segment.seconds == 20,
        "segment_before");
    free_ast_node(node);
    parse("geo_within_radius(100, 100, 10)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_GEO
            && node->special_expr.geo.op == AST_SPECIAL_GEOWITHINRADIUS
            && node->special_expr.geo.has_radius == true
            && feq(node->special_expr.geo.latitude, 100.0)
            && feq(node->special_expr.geo.longitude, 100.0)
            && feq(node->special_expr.geo.radius, 10.0),
        "geo_within_radius");
    free_ast_node(node);
    parse("geo_within_radius(100.0, 100.0, 10.0)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_GEO
            && node->special_expr.geo.op == AST_SPECIAL_GEOWITHINRADIUS
            && node->special_expr.geo.has_radius == true
            && feq(node->special_expr.geo.latitude, 100.0)
            && feq(node->special_expr.geo.longitude, 100.0)
            && feq(node->special_expr.geo.radius, 10.0),
        "geo_within_radius");
    free_ast_node(node);
    parse("contains(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_STRING
            && node->special_expr.string.op == AST_SPECIAL_CONTAINS
            && strcmp(node->special_expr.string.attr_var.attr, "a") == 0
            && strcmp(node->special_expr.string.pattern, "abc") == 0,
        "contains");
    free_ast_node(node);
    parse("starts_with(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_STRING
            && node->special_expr.string.op == AST_SPECIAL_STARTSWITH
            && strcmp(node->special_expr.string.attr_var.attr, "a") == 0
            && strcmp(node->special_expr.string.pattern, "abc") == 0,
        "starts_with");
    free_ast_node(node);
    parse("ends_with(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR && node->special_expr.type == AST_SPECIAL_STRING
            && node->special_expr.string.op == AST_SPECIAL_ENDSWITH
            && strcmp(node->special_expr.string.attr_var.attr, "a") == 0
            && strcmp(node->special_expr.string.pattern, "abc") == 0,
        "ends_with");
    free_ast_node(node);
    return 0;
}

int all_tests()
{
    mu_run_test(test_all_compare);
    mu_run_test(test_all_equality);
    mu_run_test(test_paren);
    mu_run_test(test_precedence);
    mu_run_test(test_float);
    mu_run_test(test_bool);
    mu_run_test(test_string);
    mu_run_test(test_integer_set);
    mu_run_test(test_string_set);
    mu_run_test(test_integer_list);
    mu_run_test(test_string_list);
    mu_run_test(test_special);
    mu_run_test(test_is_null);

    return 0;
}

RUN_TESTS()
