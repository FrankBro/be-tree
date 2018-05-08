#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "minunit.h"
#include "utils.h"

int parse(const char *text, struct ast_node **node);

int test_all_numeric_compare()
{
    struct ast_node* node = NULL;
    parse("a < 0", &node);
    mu_assert(node->type == AST_TYPE_NUMERIC_COMPARE_EXPR && node->numeric_compare_expr.op == AST_NUMERIC_COMPARE_LT, "LT");
    free_ast_node(node);
    parse("a <= 0", &node);
    mu_assert(node->type == AST_TYPE_NUMERIC_COMPARE_EXPR && node->numeric_compare_expr.op == AST_NUMERIC_COMPARE_LE, "LE");
    free_ast_node(node);
    parse("a > 0", &node);
    mu_assert(node->type == AST_TYPE_NUMERIC_COMPARE_EXPR && node->numeric_compare_expr.op == AST_NUMERIC_COMPARE_GT, "GT");
    free_ast_node(node);
    parse("a >= 0", &node);
    mu_assert(node->type == AST_TYPE_NUMERIC_COMPARE_EXPR && node->numeric_compare_expr.op == AST_NUMERIC_COMPARE_GE, "GE");
    free_ast_node(node);
    return 0;
}

int test_all_equality()
{
    struct ast_node* node = NULL;
    parse("a = 0", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_EQ, "EQ");
    free_ast_node(node);
    parse("a <> 0", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && node->equality_expr.op == AST_EQUALITY_NE, "NE");
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

// int test_precedence()
// {
//     struct ast_node* node = NULL;
//     parse("(a = 0 || b = 0 && c = 0 || d = 0", &node);
// }

// int test_to_string()
// {
//     struct ast_node* node = NULL;

//     {
//         const char* expr = "a = 0 && b <> 0 && c > 0 || d >= 0 || e < 0 && f <= 0";
//         parse(expr, &node);
//         const char* to_string = ast_to_string(node);
//         mu_assert(strcasecmp(expr, to_string) == 0, "");
//         free_ast_node(node);
//         free((char*)to_string);
//     }

//     return 0;
// }

int test_float()
{
    struct ast_node* node = NULL;
    parse("a = 0.", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && 
        node->equality_expr.value.value_type == AST_EQUALITY_VALUE_FLOAT && 
        feq(node->equality_expr.value.float_value, 0.)
    , "no decimal");
    free_ast_node(node);
    parse("a = 0.0", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR && 
        node->equality_expr.value.value_type == AST_EQUALITY_VALUE_FLOAT && 
        feq(node->equality_expr.value.float_value, 0.)
    , "with decimal");
    free_ast_node(node);
    return 0;
}

int test_bool()
{
    struct ast_node* node = NULL;
    parse("a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR &&
        node->bool_expr.op == AST_BOOL_VARIABLE
    , "none");
    free_ast_node(node);
    parse("not a", &node);
    mu_assert(node->type == AST_TYPE_BOOL_EXPR &&
        node->bool_expr.op == AST_BOOL_NOT
    , "not");
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
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR &&
        node->equality_expr.op == AST_EQUALITY_EQ &&
        node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING &&
        strcmp(node->equality_expr.value.string_value.string, "a") == 0 &&
        strcmp(node->equality_expr.attr_var.attr, "a") == 0
    , "eq");
    free_ast_node(node);
    parse("a <> \"a\"", &node);
    mu_assert(node->type == AST_TYPE_EQUALITY_EXPR &&
        node->equality_expr.op == AST_EQUALITY_NE &&
        node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING &&
        strcmp(node->equality_expr.value.string_value.string, "a") == 0 &&
        strcmp(node->equality_expr.attr_var.attr, "a") == 0
    , "ne");
    free_ast_node(node);
    return 0;
}

int test_integer_set()
{
    struct ast_node* node = NULL;
    parse("a in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST
    , "in");
    free_ast_node(node);
    parse("a not in (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_NOT_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST
    , "in");
    free_ast_node(node);
    parse("a in (1)", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST
    , "single");
    free_ast_node(node);
    parse("1 in a", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_INTEGER &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE
    , "flipped");
    free_ast_node(node);
    return 0;
}

int test_string_set()
{
    struct ast_node* node = NULL;
    parse("a in (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST
    , "in");
    free_ast_node(node);
    parse("a not in (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_NOT_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST
    , "in");
    free_ast_node(node);
    parse("a in (\"1\")", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST
    , "single");
    free_ast_node(node);
    parse("\"1\" in a", &node);
    mu_assert(node->type == AST_TYPE_SET_EXPR &&
        node->set_expr.op == AST_SET_IN &&
        node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING &&
        node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE
    , "flipped");
    free_ast_node(node);
    return 0;
}

int test_integer_list()
{
    struct ast_node* node = NULL;
    parse("a one of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_ONE_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST
    , "one of");
    free_ast_node(node);
    parse("a none of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_NONE_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST
    , "none of");
    free_ast_node(node);
    parse("a all of (1,2, 3)", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_ALL_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST
    , "all of");
    free_ast_node(node);
    return 0;
}

int test_string_list()
{
    struct ast_node* node = NULL;
    parse("a one of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_ONE_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST
    , "one of");
    free_ast_node(node);
    parse("a none of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_NONE_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST
    , "none of");
    free_ast_node(node);
    parse("a all of (\"1\",\"2\", \"3\")", &node);
    mu_assert(node->type == AST_TYPE_LIST_EXPR &&
        node->list_expr.op == AST_LIST_ALL_OF &&
        node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST
    , "all of");
    free_ast_node(node);
    return 0;
}

int test_special()
{
    struct ast_node* node = NULL;
    parse("within_frequency_cap(\"campaign\", \"namespace\", 100, 0)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_FREQUENCY &&
        node->special_expr.frequency.op == AST_SPECIAL_WITHINFREQUENCYCAP &&
        node->special_expr.frequency.type == FREQUENCY_TYPE_CAMPAIGN &&
        strcmp(node->special_expr.frequency.ns.string, "namespace") == 0 &&
        node->special_expr.frequency.value == 100 &&
        node->special_expr.frequency.length == 0
    , "within_frequency_cap");
    free_ast_node(node);
    parse("segment_within(1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_SEGMENT &&
        node->special_expr.segment.op == AST_SPECIAL_SEGMENTWITHIN &&
        node->special_expr.segment.has_variable == false &&
        node->special_expr.segment.segment_id == 1 &&
        node->special_expr.segment.seconds == 20
    , "segment_within");
    free_ast_node(node);
    parse("segment_before(1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_SEGMENT &&
        node->special_expr.segment.op == AST_SPECIAL_SEGMENTBEFORE &&
        node->special_expr.segment.has_variable == false &&
        node->special_expr.segment.segment_id == 1 &&
        node->special_expr.segment.seconds == 20
    , "segment_before");
    free_ast_node(node);
    parse("segment_within(name, 1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_SEGMENT &&
        node->special_expr.segment.op == AST_SPECIAL_SEGMENTWITHIN &&
        node->special_expr.segment.has_variable == true &&
        strcmp(node->special_expr.segment.attr_var.attr, "name") == 0&&
        node->special_expr.segment.segment_id == 1 &&
        node->special_expr.segment.seconds == 20
    , "segment_within");
    free_ast_node(node);
    parse("segment_before(name, 1, 20)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_SEGMENT &&
        node->special_expr.segment.op == AST_SPECIAL_SEGMENTBEFORE &&
        node->special_expr.segment.has_variable == true &&
        strcmp(node->special_expr.segment.attr_var.attr, "name") == 0 &&
        node->special_expr.segment.segment_id == 1 &&
        node->special_expr.segment.seconds == 20
    , "segment_before");
    free_ast_node(node);
    parse("geo_within_radius(100, 100, 10)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_GEO &&
        node->special_expr.geo.op == AST_SPECIAL_GEOWITHINRADIUS &&
        node->special_expr.geo.has_radius == true &&
        node->special_expr.geo.latitude.value_type == AST_SPECIAL_GEO_VALUE_INTEGER &&
        node->special_expr.geo.latitude.integer_value == 100 &&
        node->special_expr.geo.longitude.value_type == AST_SPECIAL_GEO_VALUE_INTEGER &&
        node->special_expr.geo.longitude.integer_value == 100 &&
        node->special_expr.geo.radius.value_type == AST_SPECIAL_GEO_VALUE_INTEGER &&
        node->special_expr.geo.radius.integer_value == 10
    , "geo_within_radius");
    free_ast_node(node);
    parse("geo_within_radius(100.0, 100.0, 10.0)", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_GEO &&
        node->special_expr.geo.op == AST_SPECIAL_GEOWITHINRADIUS &&
        node->special_expr.geo.has_radius == true &&
        node->special_expr.geo.latitude.value_type == AST_SPECIAL_GEO_VALUE_FLOAT &&
        feq(node->special_expr.geo.latitude.float_value, 100.0) &&
        node->special_expr.geo.longitude.value_type == AST_SPECIAL_GEO_VALUE_FLOAT &&
        feq(node->special_expr.geo.longitude.float_value, 100.0) &&
        node->special_expr.geo.radius.value_type == AST_SPECIAL_GEO_VALUE_FLOAT &&
        feq(node->special_expr.geo.radius.float_value, 10.0)
    , "geo_within_radius");
    free_ast_node(node);
    parse("contains(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_STRING &&
        node->special_expr.string.op == AST_SPECIAL_CONTAINS &&
        strcmp(node->special_expr.string.attr_var.attr, "a") == 0 &&
        strcmp(node->special_expr.string.pattern, "abc") == 0
    , "contains");
    free_ast_node(node);
    parse("starts_with(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_STRING &&
        node->special_expr.string.op == AST_SPECIAL_STARTSWITH &&
        strcmp(node->special_expr.string.attr_var.attr, "a") == 0 &&
        strcmp(node->special_expr.string.pattern, "abc") == 0
    , "starts_with");
    free_ast_node(node);
    parse("ends_with(a, \"abc\")", &node);
    mu_assert(node->type == AST_TYPE_SPECIAL_EXPR &&
        node->special_expr.type == AST_SPECIAL_STRING &&
        node->special_expr.string.op == AST_SPECIAL_ENDSWITH &&
        strcmp(node->special_expr.string.attr_var.attr, "a") == 0 &&
        strcmp(node->special_expr.string.pattern, "abc") == 0
    , "ends_with");
    free_ast_node(node);
    return 0;
}      

int all_tests() 
{
    mu_run_test(test_all_numeric_compare);
    mu_run_test(test_all_equality);
    mu_run_test(test_paren);
    // mu_run_test(test_precedence);
    // mu_run_test(test_to_string);
    mu_run_test(test_float);
    mu_run_test(test_bool);
    mu_run_test(test_string);
    mu_run_test(test_integer_set);
    mu_run_test(test_string_set);
    mu_run_test(test_integer_list);
    mu_run_test(test_string_list);
    mu_run_test(test_special);

    return 0;
}

RUN_TESTS()
