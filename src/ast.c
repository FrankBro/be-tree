#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "printer.h"
#include "special.h"
#include "utils.h"
#include "value.h"
#include "var.h"

struct ast_node* ast_node_create()
{
    struct ast_node* node = calloc(1, sizeof(*node));
    if(node == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    node->id = UINT64_MAX;
    return node;
}

struct ast_node* ast_numeric_compare_expr_create(
    enum ast_numeric_compare_e op, const char* name, struct numeric_compare_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_NUMERIC_COMPARE_EXPR;
    node->numeric_compare_expr.op = op;
    node->numeric_compare_expr.attr_var = make_attr_var(name, NULL);
    node->numeric_compare_expr.value = value;
    return node;
}

struct ast_node* ast_equality_expr_create(
    enum ast_equality_e op, const char* name, struct equality_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_EQUALITY_EXPR;
    node->equality_expr.op = op;
    node->equality_expr.attr_var = make_attr_var(name, NULL);
    node->equality_expr.value = value;
    return node;
}

static struct ast_node* ast_bool_expr_create(enum ast_bool_e op)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_BOOL_EXPR;
    node->bool_expr.op = op;
    return node;
}

struct ast_node* ast_bool_expr_variable_create(const char* name)
{
    struct ast_node* node = ast_bool_expr_create(AST_BOOL_VARIABLE);
    node->bool_expr.variable = make_attr_var(name, NULL);
    return node;
}

struct ast_node* ast_bool_expr_unary_create(struct ast_node* expr)
{
    struct ast_node* node = ast_bool_expr_create(AST_BOOL_NOT);
    node->bool_expr.unary.expr = expr;
    return node;
}

uint64_t score_node(struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            // instant
            return 1;
        case AST_TYPE_EQUALITY_EXPR:
            // instant
            return 1;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    // lhs + rhs
                    return score_node(node->bool_expr.binary.lhs) + score_node(node->bool_expr.binary.rhs);
                case AST_BOOL_NOT:
                    // expr
                    return score_node(node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    // instant
                    return 1;
                default:
                    abort();
            }
        case AST_TYPE_SET_EXPR:
            // depend on constant length
            return 5;
        case AST_TYPE_LIST_EXPR:
            // depend on constant and variable length
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF:
                    return 10;
                case AST_LIST_NONE_OF:
                    return 10;
                case AST_LIST_ALL_OF:
                    return 10;
                default:
                    abort();
            }
        case AST_TYPE_SPECIAL_EXPR:
            switch (node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    // depend on variable length
                    return 20;
                case AST_SPECIAL_SEGMENT:
                    // depend on variable length
                    return 20;
                case AST_SPECIAL_GEO:
                    // instant
                    return 20;
                case AST_SPECIAL_STRING:
                    // depend on constant and variable length
                    return 20;
                default:
                    abort();
            }
        default:
            abort();
    }
}

struct ast_node* ast_bool_expr_binary_create(
    enum ast_bool_e op, struct ast_node* lhs, struct ast_node* rhs)
{
    struct ast_node* node = ast_bool_expr_create(op);
    uint64_t lhs_score = score_node(lhs);
    uint64_t rhs_score = score_node(rhs);
    if(rhs_score < lhs_score) {
        node->bool_expr.binary.lhs = rhs;
        node->bool_expr.binary.rhs = lhs;
    }
    else {
        node->bool_expr.binary.lhs = lhs;
        node->bool_expr.binary.rhs = rhs;
    }
    return node;
}

struct ast_node* ast_set_expr_create(
    enum ast_set_e op, struct set_left_value left_value, struct set_right_value right_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_SET_EXPR;
    node->set_expr.op = op;
    node->set_expr.left_value = left_value;
    node->set_expr.right_value = right_value;
    return node;
}

struct ast_node* ast_list_expr_create(
    enum ast_list_e op, const char* name, struct list_value list_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_LIST_EXPR;
    node->list_expr.op = op;
    node->list_expr.attr_var = make_attr_var(name, NULL);
    node->list_expr.value = list_value;
    return node;
}

struct ast_node* ast_special_expr_create()
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_SPECIAL_EXPR;
    return node;
}

struct ast_node* ast_special_frequency_create(const enum ast_special_frequency_e op,
    const char* stype,
    struct string_value ns,
    int64_t value,
    size_t length)
{
    enum frequency_type_e type = get_type_from_string(stype);
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_frequency frequency = { .op = op,
        .attr_var = make_attr_var("frequency_caps", NULL),
        .type = type,
        .ns = ns,
        .value = value,
        .length = length };
    node->special_expr.type = AST_SPECIAL_FREQUENCY;
    node->special_expr.frequency = frequency;
    node->special_expr.frequency.now = make_attr_var("now", NULL);
    return node;
}

struct ast_node* ast_special_segment_create(
    const enum ast_special_segment_e op, const char* name, betree_seg_t segment_id, int64_t seconds)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_segment segment = { .op = op, .segment_id = segment_id, .seconds = seconds };
    if(name == NULL) {
        segment.has_variable = false;
        segment.attr_var = make_attr_var("segments_with_timestamp", NULL);
    }
    else {
        segment.has_variable = true;
        segment.attr_var = make_attr_var(name, NULL);
    }
    node->special_expr.type = AST_SPECIAL_SEGMENT;
    node->special_expr.segment = segment;
    node->special_expr.segment.now = make_attr_var("now", NULL);
    return node;
}

struct ast_node* ast_special_geo_create(const enum ast_special_geo_e op,
    struct special_geo_value latitude,
    struct special_geo_value longitude,
    bool has_radius,
    struct special_geo_value radius)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_geo geo = { .op = op,
        .latitude = latitude,
        .longitude = longitude,
        .has_radius = has_radius,
        .radius = radius,
        .latitude_var = make_attr_var("latitude", NULL),
        .longitude_var = make_attr_var("longitude", NULL) };
    node->special_expr.type = AST_SPECIAL_GEO;
    node->special_expr.geo = geo;
    return node;
}

struct ast_node* ast_special_string_create(
    const enum ast_special_string_e op, const char* name, const char* pattern)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_string string
        = { .op = op, .attr_var = make_attr_var(name, NULL), .pattern = strdup(pattern) };
    node->special_expr.type = AST_SPECIAL_STRING;
    node->special_expr.string = string;
    return node;
}

void free_special_expr(struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY:
            free_attr_var(special_expr.frequency.attr_var);
            free_attr_var(special_expr.frequency.now);
            free((char*)special_expr.frequency.ns.string);
            break;
        case AST_SPECIAL_SEGMENT:
            free_attr_var(special_expr.segment.attr_var);
            free_attr_var(special_expr.segment.now);
            break;
        case AST_SPECIAL_GEO:
            free_attr_var(special_expr.geo.latitude_var);
            free_attr_var(special_expr.geo.longitude_var);
            break;
        case AST_SPECIAL_STRING:
            free_attr_var(special_expr.string.attr_var);
            free((char*)special_expr.string.pattern);
            break;
        default:
            switch_default_error("Invalid special expr type");
    }
}

void free_set_expr(struct ast_set_expr set_expr)
{
    switch(set_expr.left_value.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER: {
            break;
        }
        case AST_SET_LEFT_VALUE_STRING: {
            free((char*)set_expr.left_value.string_value.string);
            break;
        }
        case AST_SET_LEFT_VALUE_VARIABLE: {
            free_attr_var(set_expr.left_value.variable_value);
            break;
        }
        default: {
            switch_default_error("Invalid set left value type");
        }
    }
    switch(set_expr.right_value.value_type) {
        case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
            free(set_expr.right_value.integer_list_value.integers);
            break;
        }
        case AST_SET_RIGHT_VALUE_STRING_LIST: {
            for(size_t i = 0; i < set_expr.right_value.string_list_value.count; i++) {
                free((char*)set_expr.right_value.string_list_value.strings[i].string);
            }
            free(set_expr.right_value.string_list_value.strings);
            break;
        }
        case AST_SET_RIGHT_VALUE_VARIABLE: {
            free_attr_var(set_expr.right_value.variable_value);
            break;
        }
        default: {
            switch_default_error("Invalid set right value type");
        }
    }
}

void free_list_expr(struct ast_list_expr list_expr)
{
    switch(list_expr.value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            free(list_expr.value.integer_list_value.integers);
            break;
        }
        case AST_LIST_VALUE_STRING_LIST: {
            for(size_t i = 0; i < list_expr.value.string_list_value.count; i++) {
                free((char*)list_expr.value.string_list_value.strings[i].string);
            }
            free(list_expr.value.string_list_value.strings);
            break;
        }
        default: {
            switch_default_error("Invalid list value type");
        }
    }
    free_attr_var(list_expr.attr_var);
}

void free_ast_node(struct ast_node* node)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR:
            free_special_expr(node->special_expr);
            break;
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            free_attr_var(node->numeric_compare_expr.attr_var);
            break;
        case AST_TYPE_EQUALITY_EXPR:
            free_attr_var(node->equality_expr.attr_var);
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                free((char*)node->equality_expr.value.string_value.string);
            }
            break;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_NOT:
                    free_ast_node(node->bool_expr.unary.expr);
                    break;
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    free_ast_node(node->bool_expr.binary.lhs);
                    free_ast_node(node->bool_expr.binary.rhs);
                    break;
                case AST_BOOL_VARIABLE:
                    free_attr_var(node->bool_expr.variable);
                    break;
                default:
                    switch_default_error("Invalid bool expr operation");
                    break;
            }
            break;
        case AST_TYPE_SET_EXPR:
            free_set_expr(node->set_expr);
            break;
        case AST_TYPE_LIST_EXPR:
            free_list_expr(node->list_expr);
            break;
        default: {
            switch_default_error("Invalid expr type");
        }
    }
    free(node);
}

static void invalid_expr(const char* msg)
{
    fprintf(stderr, "%s", msg);
    abort();
}

bool d64binary_search(int64_t arr[], int min, int max, int64_t to_find)
{
   if(max >= min)
   {
        int mid = min + (max - min)/2;

        if (arr[mid] == to_find) {
            return true;
        }

        if (arr[mid] > to_find) {
            return d64binary_search(arr, min, mid-1, to_find);
        }

        return d64binary_search(arr, mid+1, max, to_find);
   }

   return false;
}

bool sbinary_search(struct string_value arr[], int min, int max, betree_str_t to_find)
{
   if(max >= min)
   {
        int mid = min + (max - min)/2;

        if (arr[mid].str == to_find) {
            return true;
        }

        if (arr[mid].str > to_find) {
            return sbinary_search(arr, min, mid-1, to_find);
        }

        return sbinary_search(arr, mid+1, max, to_find);
   }

   return false;
}

bool integer_in_integer_list(int64_t integer, struct integer_list_value list)
{
    return d64binary_search(list.integers, 0, list.count, integer);
}

bool string_in_string_list(struct string_value string, struct string_list_value list)
{
    return sbinary_search(list.strings, 0, list.count, string.str);
}

bool numeric_compare_value_matches(enum ast_numeric_compare_value_e a, enum value_e b)
{
    return (a == AST_NUMERIC_COMPARE_VALUE_INTEGER && b == VALUE_I)
        || (a == AST_NUMERIC_COMPARE_VALUE_FLOAT && b == VALUE_F);
}

bool equality_value_matches(enum ast_equality_value_e a, enum value_e b)
{
    return (a == AST_EQUALITY_VALUE_INTEGER && b == VALUE_I)
        || (a == AST_EQUALITY_VALUE_FLOAT && b == VALUE_F)
        || (a == AST_EQUALITY_VALUE_STRING && b == VALUE_S);
}

bool list_value_matches(enum ast_list_value_e a, enum value_e b)
{
    return (a == AST_LIST_VALUE_INTEGER_LIST && b == VALUE_IL)
        || (a == AST_LIST_VALUE_STRING_LIST && b == VALUE_SL);
}

double get_geo_value_as_float(const struct special_geo_value value)
{
    switch(value.value_type) {
        case AST_SPECIAL_GEO_VALUE_INTEGER: {
            return (double)value.integer_value;
        }
        case AST_SPECIAL_GEO_VALUE_FLOAT: {
            return value.float_value;
        }
        default: {
            switch_default_error("Invalid geo value type");
            return false;
        }
    }
}

const char* frequency_type_to_string(enum frequency_type_e type)
{
    const char* string;
    switch(type) {
        case FREQUENCY_TYPE_ADVERTISER: {
            string = "advertiser";
            break;
        }
        case FREQUENCY_TYPE_ADVERTISERIP: {
            string = "advertiser:ip";
            break;
        }
        case FREQUENCY_TYPE_CAMPAIGN: {
            string = "campaign";
            break;
        }
        case FREQUENCY_TYPE_CAMPAIGNIP: {
            string = "campaign:ip";
            break;
        }
        case FREQUENCY_TYPE_FLIGHT: {
            string = "flight";
            break;
        }
        case FREQUENCY_TYPE_FLIGHTIP: {
            string = "flight:ip";
            break;
        }
        case FREQUENCY_TYPE_PRODUCT: {
            string = "product";
            break;
        }
        case FREQUENCY_TYPE_PRODUCTIP: {
            string = "product:ip";
            break;
        }
        default: {
            switch_default_error("Invalid frequency type");
            abort();
        }
    }
    return string;
}

bool match_special_expr(
    const struct config* config, const struct pred** preds, const struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY: {
            switch(special_expr.frequency.op) {
                case AST_SPECIAL_WITHINFREQUENCYCAP: {
                    int64_t now;
                    enum variable_state_e state = get_integer_var(config, special_expr.frequency.now.var, preds, &now);
                    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
                    if(state == VARIABLE_UNDEFINED) {
                        return false;
                    }
                    struct frequency_caps_list caps;
                    enum variable_state_e caps_state = get_frequency_var(config, special_expr.frequency.attr_var.var, preds, &caps);
                    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
                    if(caps_state == VARIABLE_UNDEFINED) {
                        return false;
                    }
                    else {
                        uint32_t id = 20;
                        switch(special_expr.frequency.type) {
                            case FREQUENCY_TYPE_ADVERTISER:
                            case FREQUENCY_TYPE_ADVERTISERIP:
                                // id = ADVERTISER_ID;
                                id = 20;
                                break;
                            case FREQUENCY_TYPE_CAMPAIGN:
                            case FREQUENCY_TYPE_CAMPAIGNIP:
                                // id = CAMPAIGN_ID;
                                id = 30;
                                break;
                            case FREQUENCY_TYPE_FLIGHT:
                            case FREQUENCY_TYPE_FLIGHTIP:
                                // id = FLIGHT_ID;
                                id = 10;
                                break;
                            case FREQUENCY_TYPE_PRODUCT:
                            case FREQUENCY_TYPE_PRODUCTIP:
                                // id = PRODUCT_ID;
                                id = 40;
                                break;
                            default:
                                switch_default_error("Invalid frequency type");
                                break;
                        }
                        return within_frequency_caps(config,
                            &caps,
                            special_expr.frequency.type,
                            id,
                            special_expr.frequency.ns,
                            special_expr.frequency.value,
                            special_expr.frequency.length,
                            now);
                    }
                }
                default: {
                    switch_default_error("Invalid frequency operation");
                    return false;
                }
            }
        }
        case AST_SPECIAL_SEGMENT: {
            int64_t now;
            enum variable_state_e now_state = get_integer_var(config, special_expr.segment.now.var, preds, &now);
            betree_assert(config->abort_on_error, ERROR_VAR_MISSING, now_state != VARIABLE_MISSING);
            struct segments_list segments;
            enum variable_state_e segments_state
                = get_segments_var(config, special_expr.segment.attr_var.var, preds, &segments);
            betree_assert(config->abort_on_error, ERROR_VAR_MISSING, segments_state != VARIABLE_MISSING);
            if(now_state == VARIABLE_UNDEFINED || segments_state == VARIABLE_UNDEFINED) {
                return false;
            }
            switch(special_expr.segment.op) {
                case AST_SPECIAL_SEGMENTWITHIN: {
                    return segment_within(special_expr.segment.segment_id,
                        special_expr.segment.seconds,
                        &segments,
                        now);
                }
                case AST_SPECIAL_SEGMENTBEFORE: {
                    return segment_before(special_expr.segment.segment_id,
                        special_expr.segment.seconds,
                        &segments,
                        now);
                }
                default: {
                    switch_default_error("Invalid segment operation");
                    return false;
                }
            }
        }
        case AST_SPECIAL_GEO: {
            switch(special_expr.geo.op) {
                case AST_SPECIAL_GEOWITHINRADIUS: {
                    double latitude_var, longitude_var;
                    enum variable_state_e latitude_var_state
                        = get_float_var(config, special_expr.geo.latitude_var.var, preds, &latitude_var);
                    enum variable_state_e longitude_var_state
                        = get_float_var(config, special_expr.geo.longitude_var.var, preds, &longitude_var);
                    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, latitude_var_state != VARIABLE_MISSING);
                    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, longitude_var_state != VARIABLE_MISSING);
                    if(latitude_var_state == VARIABLE_UNDEFINED
                        || longitude_var_state == VARIABLE_UNDEFINED) {
                        return false;
                    }
                    double latitude_cst = get_geo_value_as_float(special_expr.geo.latitude);
                    double longitude_cst = get_geo_value_as_float(special_expr.geo.longitude);
                    double radius_cst = get_geo_value_as_float(special_expr.geo.radius);

                    return geo_within_radius(
                        latitude_cst, longitude_cst, latitude_var, longitude_var, radius_cst);
                }
                default: {
                    switch_default_error("Invalid geo operation");
                    return false;
                }
            }
            return false;
        }
        case AST_SPECIAL_STRING: {
            struct string_value value;
            enum variable_state_e state
                = get_string_var(config, special_expr.string.attr_var.var, preds, &value);
            betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
            if(state == VARIABLE_UNDEFINED) {
                return false;
            }
            switch(special_expr.string.op) {
                case AST_SPECIAL_CONTAINS: {
                    return contains(value.string, special_expr.string.pattern);
                }
                case AST_SPECIAL_STARTSWITH: {
                    return starts_with(value.string, special_expr.string.pattern);
                }
                case AST_SPECIAL_ENDSWITH: {
                    return ends_with(value.string, special_expr.string.pattern);
                }
                default: {
                    switch_default_error("Invalid string operation");
                    return false;
                }
            }
            return false;
        }
        default:
            switch_default_error("Invalid special expr type");
            return false;
    }
}

static bool match_not_all_of_int(struct value variable, struct ast_list_expr list_expr)
{
    int64_t* xs;
    size_t x_count;
    int64_t* ys;
    size_t y_count;
    if(variable.ilvalue.count < list_expr.value.integer_list_value.count) {
        xs = variable.ilvalue.integers;
        x_count = variable.ilvalue.count;
        ys = list_expr.value.integer_list_value.integers;
        y_count = list_expr.value.integer_list_value.count;
    }
    else {
        ys = variable.ilvalue.integers;
        y_count = variable.ilvalue.count;
        xs = list_expr.value.integer_list_value.integers;
        x_count = list_expr.value.integer_list_value.count;
    }
    size_t i = 0, j = 0;
    while(i < x_count && j < y_count) {
        int64_t x = xs[i];
        int64_t y = ys[j];
        if(x == y) {
            return true;
        }
        else if(y < x) {
            j++;
        }
        else {
            i++;
        }
    }
    return false;
}

static bool match_not_all_of_string(struct value variable, struct ast_list_expr list_expr)
{
    struct string_value* xs;
    size_t x_count;
    struct string_value* ys;
    size_t y_count;
    if(variable.slvalue.count < list_expr.value.integer_list_value.count) {
        xs = variable.slvalue.strings;
        x_count = variable.slvalue.count;
        ys = list_expr.value.string_list_value.strings;
        y_count = list_expr.value.integer_list_value.count;
    }
    else {
        ys = variable.slvalue.strings;
        y_count = variable.slvalue.count;
        xs = list_expr.value.string_list_value.strings;
        x_count = list_expr.value.integer_list_value.count;
    }
    size_t i = 0, j = 0;
    while(i < x_count && j < y_count) {
        struct string_value* x = &xs[i];
        struct string_value* y = &ys[j];
        if(x->str == y->str) {
            return true;
        }
        else if(y->str < x->str) {
            j++;
        }
        else {
            i++;
        }
    }
    return false;
}

static bool match_all_of_int(struct value variable, struct ast_list_expr list_expr)
{
    int64_t* xs = list_expr.value.integer_list_value.integers;
    size_t x_count = list_expr.value.integer_list_value.count;
    int64_t* ys = variable.ilvalue.integers;
    size_t y_count = variable.ilvalue.count;
    if(x_count <= y_count) {
        size_t i = 0, j = 0;
        while(i < y_count && j < x_count) {
            int64_t x = xs[j];
            int64_t y = ys[i];
            if(y < x) {
                i++;
            }
            else if(x == y) {
                i++;
                j++;
            }
            else {
                return false;
            }
        }
        if(j < x_count) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

static bool match_all_of_string(struct value variable, struct ast_list_expr list_expr)
{
    struct string_value* xs = list_expr.value.string_list_value.strings;
    size_t x_count = list_expr.value.string_list_value.count;
    struct string_value* ys = variable.slvalue.strings;
    size_t y_count = variable.slvalue.count;
    if(x_count <= y_count) {
        size_t i = 0, j = 0;
        while(i < y_count && j < x_count) {
            struct string_value* x = &xs[j];
            struct string_value* y = &ys[i];
            if(y->str < x->str) {
                i++;
            }
            else if(x->str == y->str) {
                i++;
                j++;
            }
            else {
                return false;
            }
        }
        if(j < x_count) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

bool match_list_expr(
    const struct config* config, const struct pred** preds, const struct ast_list_expr list_expr)
{
    struct value variable;
    enum variable_state_e state = get_variable(config, list_expr.attr_var.var, preds, &variable);
    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(config->abort_on_error, ERROR_LIST_TYPE_MISMATCH, list_value_matches(list_expr.value.value_type, variable.value_type));
    switch(list_expr.op) {
        case AST_LIST_ONE_OF:
        case AST_LIST_NONE_OF: {
            bool result = false;
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: 
                    result = match_not_all_of_int(variable, list_expr);
                    break;
                case AST_LIST_VALUE_STRING_LIST: {
                    result = match_not_all_of_string(variable, list_expr);
                    break;
                }
                default: {
                    switch_default_error("Invalid list value type");
                }
            }
            switch(list_expr.op) {
                case AST_LIST_ONE_OF:
                    return result;
                case AST_LIST_NONE_OF:
                    return !result;
                case AST_LIST_ALL_OF:
                    invalid_expr("Should never happen");
                    return false;
                default: {
                    switch_default_error("Invalid list operation");
                    return false;
                }
            }
        }
        case AST_LIST_ALL_OF: {
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST:
                    return match_all_of_int(variable, list_expr);
                case AST_LIST_VALUE_STRING_LIST:
                    return match_all_of_string(variable, list_expr);
                default: {
                    switch_default_error("Invalid list value type");
                    return false;
                }
            }
        }
        default: {
            switch_default_error("Invalid list operation");
            return false;
        }
    }
}

bool match_set_expr(
    const struct config* config, const struct pred** preds, const struct ast_set_expr set_expr)
{
    struct set_left_value left = set_expr.left_value;
    struct set_right_value right = set_expr.right_value;
    bool is_in;
    if(left.value_type == AST_SET_LEFT_VALUE_INTEGER
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct integer_list_value variable;
        enum variable_state_e state
            = get_integer_list_var(config, right.variable_value.var, preds, &variable);
        betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = integer_in_integer_list(left.integer_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_STRING
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct string_list_value variable;
        enum variable_state_e state
            = get_string_list_var(config, right.variable_value.var, preds, &variable);
        betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = string_in_string_list(left.string_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
        int64_t variable;
        enum variable_state_e state
            = get_integer_var(config, left.variable_value.var, preds, &variable);
        betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = integer_in_integer_list(variable, right.integer_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
        struct string_value variable;
        enum variable_state_e state
            = get_string_var(config, left.variable_value.var, preds, &variable);
        betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = string_in_string_list(variable, right.string_list_value);
    }
    else {
        invalid_expr("invalid set expression");
        return false;
    }
    switch(set_expr.op) {
        case AST_SET_NOT_IN: {
            return !is_in;
        }
        case AST_SET_IN: {
            return is_in;
        }
        default: {
            switch_default_error("Invalid set operation");
            return false;
        }
    }
}

bool match_numeric_compare_expr(const struct config* config,
    const struct pred** preds,
    const struct ast_numeric_compare_expr numeric_compare_expr)
{
    struct value variable;
    enum variable_state_e state
        = get_variable(config, numeric_compare_expr.attr_var.var, preds, &variable);
    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, numeric_compare_value_matches(numeric_compare_expr.value.value_type, variable.value_type));
    switch(numeric_compare_expr.op) {
        case AST_NUMERIC_COMPARE_LT: {
            switch(numeric_compare_expr.value.value_type) {
                case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue < numeric_compare_expr.value.integer_value;
                    return result;
                }
                case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue < numeric_compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid numeric compare value type");
                    return false;
                }
            }
        }
        case AST_NUMERIC_COMPARE_LE: {
            switch(numeric_compare_expr.value.value_type) {
                case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue <= numeric_compare_expr.value.integer_value;
                    return result;
                }
                case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue <= numeric_compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid numeric compare value type");
                    return false;
                }
            }
        }
        case AST_NUMERIC_COMPARE_GT: {
            switch(numeric_compare_expr.value.value_type) {
                case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue > numeric_compare_expr.value.integer_value;
                    return result;
                }
                case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue > numeric_compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid numeric compare value type");
                    return false;
                }
            }
        }
        case AST_NUMERIC_COMPARE_GE: {
            switch(numeric_compare_expr.value.value_type) {
                case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue >= numeric_compare_expr.value.integer_value;
                    return result;
                }
                case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue >= numeric_compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid numeric compare value type");
                    return false;
                }
            }
        }
        default: {
            switch_default_error("Invalid numeric compare operation");
            return false;
        }
    }
}

bool match_equality_expr(const struct config* config,
    const struct pred** preds,
    const struct ast_equality_expr equality_expr)
{
    struct value variable;
    enum variable_state_e state
        = get_variable(config, equality_expr.attr_var.var, preds, &variable);
    betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, equality_value_matches(equality_expr.value.value_type, variable.value_type));
    switch(equality_expr.op) {
        case AST_EQUALITY_EQ: {
            switch(equality_expr.value.value_type) {
                case AST_EQUALITY_VALUE_INTEGER: {
                    bool result = variable.ivalue == equality_expr.value.integer_value;
                    return result;
                }
                case AST_EQUALITY_VALUE_FLOAT: {
                    bool result = feq(variable.fvalue, equality_expr.value.float_value);
                    return result;
                }
                case AST_EQUALITY_VALUE_STRING: {
                    betree_assert(config->abort_on_error, ERROR_STRING_VAR_MISMATCH, variable.svalue.var == equality_expr.value.string_value.var);
                    bool result = variable.svalue.str == equality_expr.value.string_value.str;
                    return result;
                }
                default: {
                    switch_default_error("Invalid equality value type");
                    return false;
                }
            }
        }
        case AST_EQUALITY_NE: {
            switch(equality_expr.value.value_type) {
                case AST_EQUALITY_VALUE_INTEGER: {
                    bool result = variable.ivalue != equality_expr.value.integer_value;
                    return result;
                }
                case AST_EQUALITY_VALUE_FLOAT: {
                    bool result = fne(variable.fvalue, equality_expr.value.float_value);
                    return result;
                }
                case AST_EQUALITY_VALUE_STRING: {
                    betree_assert(config->abort_on_error, ERROR_STRING_VAR_MISMATCH, variable.svalue.var == equality_expr.value.string_value.var);
                    bool result = variable.svalue.str != equality_expr.value.string_value.str;
                    return result;
                }
                default: {
                    switch_default_error("Invalid equality value type");
                    return false;
                }
            }
        }
        default: {
            switch_default_error("Invalid equality operation");
            return false;
        }
    }
}

static bool match_node_inner(const struct config* config, const struct pred** preds, const struct ast_node* node, struct memoize* memoize, struct report* report);

bool match_bool_expr(
    const struct config* config, const struct pred** preds, const struct ast_bool_expr bool_expr, struct memoize* memoize, struct report* report)
{
    switch(bool_expr.op) {
        case AST_BOOL_AND: {
            bool lhs = match_node_inner(config, preds, bool_expr.binary.lhs, memoize, report);
            if(lhs == false) {
                return false;
            }
            bool rhs = match_node_inner(config, preds, bool_expr.binary.rhs, memoize, report);
            return rhs;
        }
        case AST_BOOL_OR: {
            bool lhs = match_node_inner(config, preds, bool_expr.binary.lhs, memoize, report);
            if(lhs == true) {
                return true;
            }
            bool rhs = match_node_inner(config, preds, bool_expr.binary.rhs, memoize, report);
            return rhs;
        }
        case AST_BOOL_NOT: {
            bool result = match_node_inner(config, preds, bool_expr.unary.expr, memoize, report);
            return !result;
        }
        case AST_BOOL_VARIABLE: {
            bool value;
            enum variable_state_e state
                = get_bool_var(config, bool_expr.variable.var, preds, &value);
            betree_assert(config->abort_on_error, ERROR_VAR_MISSING, state != VARIABLE_MISSING);
            if(state == VARIABLE_UNDEFINED) {
                return false;
            }
            return value;
        }
        default: {
            switch_default_error("Invalid bool operation");
            return false;
        }
    }
}

void report_memoized(struct report* report)
{
    if(report != NULL) {
        report->memoized++;
    }
}

bool MATCH_NODE_DEBUG = false;

void print_memoize(const struct memoize* memoize, size_t pred_count)
{
    printf("DEBUG: Pass ");
    for(size_t i = 0; i < pred_count; i++) {
        bool result = test_bit(memoize->pass, i);
        printf("%d", result);
    }
    printf("\n");
    printf("DEBUG: Fail ");
    for(size_t i = 0; i < pred_count; i++) {
        bool result = test_bit(memoize->fail, i);
        printf("%d", result);
    }
    printf("\n");
}

static bool match_node_inner(const struct config* config, const struct pred** preds, const struct ast_node* node, struct memoize* memoize, struct report* report)
{
    // TODO allow undefined handling?
    if(MATCH_NODE_DEBUG) {
        const char* memoize_status;
        /*print_memoize(memoize, config->pred_count);*/
        if(memoize != NULL) {
            if(test_bit(memoize->pass, node->id)) {
                memoize_status = "PASS";
            }
            else if(test_bit(memoize->fail, node->id)) {
                memoize_status = "FAIL";
            }
            else {
                memoize_status = "NOPE";
            }
        }
        else {
            memoize_status = "NOPE";
        }
        const char* expr = ast_to_string(node);
        printf("DEBUG: Pred: %lu, Memoize: %s, %s\n", node->id, memoize_status, expr);
        free((char*)expr);
    }
    if(memoize != NULL) {
        if(test_bit(memoize->pass, node->id)) {
            report_memoized(report);
            return true;
        }
        if(test_bit(memoize->fail, node->id)) {
            report_memoized(report);
            return false;
        }
    }
    bool result;
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            result = match_special_expr(config, preds, node->special_expr);
            break;
        }
        case AST_TYPE_BOOL_EXPR: {
            result = match_bool_expr(config, preds, node->bool_expr, memoize, report);
            break;
        }
        case AST_TYPE_LIST_EXPR: {
            result = match_list_expr(config, preds, node->list_expr);
            break;
        }
        case AST_TYPE_SET_EXPR: {
            result = match_set_expr(config, preds, node->set_expr);
            break;
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            result = match_numeric_compare_expr(config, preds, node->numeric_compare_expr);
            break;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            result = match_equality_expr(config, preds, node->equality_expr);
            break;
        }
        default: {
            switch_default_error("Invalid expr type");
            return false;
        }
    }
    if(memoize != NULL) {
        if(result) {
            set_bit(memoize->pass, node->id);
        }
        else {
            set_bit(memoize->fail, node->id);
        }
    }
    return result;
}

bool match_node(const struct config* config, const struct pred** preds, const struct ast_node* node, struct memoize* memoize, struct report* report)
{
    return match_node_inner(config, preds, node, memoize, report);
}

struct value_bound copy_value_bound(struct value_bound* bound)
{
    struct value_bound copy;
    copy.value_type = bound->value_type;
    switch(bound->value_type) {
        case VALUE_B:
            copy.bmin = bound->bmin;
            copy.bmax = bound->bmax;
            break;
        case VALUE_I:
            copy.imin = bound->imin;
            copy.imax = bound->imax;
            break;
        case VALUE_F:
            copy.fmin = bound->fmin;
            copy.fmax = bound->fmax;
            break;
        case VALUE_S:
            copy.is_string_bounded = bound->is_string_bounded;
            copy.smin = bound->smin;
            copy.smax = bound->smax;
            break;
        case VALUE_IL:
            copy.is_integer_list_bounded = bound->is_integer_list_bounded;
            copy.ilmin = bound->ilmin;
            copy.ilmax = bound->ilmax;
            break;
        case VALUE_SL:
            copy.is_string_list_bounded = bound->is_string_list_bounded;
            copy.slmin = bound->slmin;
            copy.slmax = bound->slmax;
            break;
        case VALUE_SEGMENTS:
        case VALUE_FREQUENCY:
        default:
            break;
    }
    return copy;
}

static void get_variable_bound_inner(const struct attr_domain* domain, const struct ast_node* node, struct value_bound* bound, bool is_reversed, bool* was_touched)
{
    if(node == NULL) {
        return;
    }
    bool was_touched_value = *was_touched;
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: 
            return;
        case AST_TYPE_LIST_EXPR:
            if(domain->attr_var.var != node->list_expr.attr_var.var) {
                return;
            }
            if(!list_value_matches(node->list_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain and expr type mismatch");
                return;
            }
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF:
                case AST_LIST_ALL_OF:
                    if(domain->attr_var.var != node->list_expr.attr_var.var) {
                        return;
                    }
                    if(domain->bound.value_type == VALUE_IL && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST) {
                        if(is_reversed) {
                            bound->ilmin = domain->bound.ilmin;
                            bound->ilmax = domain->bound.ilmax;
                        }
                        else {
                            if(node->list_expr.value.integer_list_value.count != 0) {
                                bound->ilmin = d64min(bound->ilmin, node->list_expr.value.integer_list_value.integers[0]);
                                bound->ilmax = d64max(bound->ilmax, node->list_expr.value.integer_list_value.integers[node->list_expr.value.integer_list_value.count-1]);
                            }
                            else {
                                return;
                            }
                        }
                        *was_touched = true;
                        return;
                    }
                    else if(domain->bound.value_type == VALUE_SL && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                        if(is_reversed) {
                            bound->slmin = domain->bound.slmin;
                            bound->slmax = domain->bound.slmax;
                        }
                        else {
                            if(node->list_expr.value.string_list_value.count != 0) {
                                bound->slmin = smin(bound->slmin, node->list_expr.value.string_list_value.strings[0].str);
                                bound->slmax = smax(bound->slmax, node->list_expr.value.string_list_value.strings[node->list_expr.value.string_list_value.count-1].str);
                            }
                            else {
                                return;
                            }
                        }
                        *was_touched = true;
                        return;
                    }
                    else {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                case AST_LIST_NONE_OF:
                    if(domain->bound.value_type == VALUE_IL && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST) {
                        if(is_reversed) {
                            if(node->list_expr.value.integer_list_value.count != 0) {
                                bound->ilmin = d64min(bound->ilmin, node->list_expr.value.integer_list_value.integers[0]);
                                bound->ilmax = d64max(bound->ilmax, node->list_expr.value.integer_list_value.integers[node->list_expr.value.integer_list_value.count-1]);
                            }
                            else {
                                return;
                            }
                        }
                        else {
                            bound->ilmin = domain->bound.ilmin;
                            bound->ilmax = domain->bound.ilmax;
                        }
                        *was_touched = true;
                        return;
                    }
                    else if(domain->bound.value_type == VALUE_SL && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                        if(is_reversed) {
                            if(node->list_expr.value.string_list_value.count != 0) {
                                bound->slmin = smin(bound->slmin, node->list_expr.value.string_list_value.strings[0].str);
                                bound->slmax = smax(bound->slmax, node->list_expr.value.string_list_value.strings[node->list_expr.value.string_list_value.count-1].str);
                            }
                            else {
                                return;
                            }
                        }
                        else {
                            bound->slmin = domain->bound.slmin;
                            bound->slmax = domain->bound.slmax;
                        }
                        *was_touched = true;
                        return;
                    }
                    else {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                default:
                    switch_default_error("Invalid set operation");
                    return;
            }
        case AST_TYPE_SET_EXPR:
            switch(node->set_expr.op) {
                case AST_SET_IN:
                    if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                        if(domain->attr_var.var != node->set_expr.left_value.variable_value.var) {
                            return;
                        }
                        if(domain->bound.value_type == VALUE_I && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
                            if(is_reversed) {
                                bound->imin = domain->bound.imin;
                                bound->imax = domain->bound.imax;
                            }
                            else {
                                if(node->set_expr.right_value.integer_list_value.count != 0) {
                                    bound->imin = d64min(bound->imin, node->set_expr.right_value.integer_list_value.integers[0]);
                                    bound->imax = d64max(bound->imax, node->set_expr.right_value.integer_list_value.integers[node->set_expr.right_value.integer_list_value.count-1]);
                                }
                                else {
                                    return;
                                }
                            }
                            *was_touched = true;
                            return;
                        }
                        else if(domain->bound.value_type == VALUE_S && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                            if(is_reversed) {
                                bound->smin = domain->bound.smin;
                                bound->smax = domain->bound.smax;
                            }
                            else {
                                if(node->set_expr.right_value.string_list_value.count != 0) {
                                    bound->smin = smin(bound->smin, node->set_expr.right_value.string_list_value.strings[0].str);
                                    bound->smax = smax(bound->smax, node->set_expr.right_value.string_list_value.strings[node->set_expr.right_value.string_list_value.count-1].str);
                                }
                                else {
                                    return;
                                }
                            }
                            *was_touched = true;
                            return;
                        }
                        else {
                            invalid_expr("Domain and expr type mismatch");
                            return;
                        }
                    }
                    else if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                        if(domain->attr_var.var != node->set_expr.right_value.variable_value.var) {
                            return;
                        }
                        if(domain->bound.value_type == VALUE_IL && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_INTEGER) {
                            if(is_reversed) {
                                bound->ilmin = domain->bound.ilmin;
                                bound->ilmax = domain->bound.ilmax;
                            }
                            else {
                                if(node->set_expr.right_value.integer_list_value.count != 0) {
                                    bound->ilmin = d64min(bound->ilmin, node->set_expr.left_value.integer_value);
                                    bound->ilmax = d64max(bound->ilmax, node->set_expr.left_value.integer_value);
                                }
                                else {
                                    return;
                                }
                            }
                            *was_touched = true;
                            return;
                        }
                        else if(domain->bound.value_type == VALUE_SL && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                            if(is_reversed) {
                                bound->slmin = domain->bound.slmin;
                                bound->slmax = domain->bound.slmax;
                            }
                            else {
                                if(node->set_expr.right_value.string_list_value.count != 0) {
                                    bound->slmin = smin(bound->slmin, node->set_expr.left_value.string_value.str);
                                    bound->slmax = smax(bound->slmax, node->set_expr.left_value.string_value.str);
                                }
                                else {
                                    return;
                                }
                            }
                            *was_touched = true;
                            return;
                        }
                        else {
                            invalid_expr("Domain and expr type mismatch");
                            return;
                        }
                    }
                    else {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                case AST_SET_NOT_IN:
                    if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                        if(domain->attr_var.var != node->set_expr.left_value.variable_value.var) {
                            return;
                        }
                        if(domain->bound.value_type == VALUE_I && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.integer_list_value.count != 0) {
                                    bound->imin = d64min(bound->imin, node->set_expr.right_value.integer_list_value.integers[0]);
                                    bound->imax = d64max(bound->imax, node->set_expr.right_value.integer_list_value.integers[node->set_expr.right_value.integer_list_value.count-1]);
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                                bound->imin = domain->bound.imin;
                                bound->imax = domain->bound.imax;
                            }
                            *was_touched = true;
                            return;
                        }
                        else if(domain->bound.value_type == VALUE_S && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.string_list_value.count != 0) {
                                    bound->smin = smin(bound->smin, node->set_expr.right_value.string_list_value.strings[0].str);
                                    bound->smax = smax(bound->smax, node->set_expr.right_value.string_list_value.strings[node->set_expr.right_value.string_list_value.count-1].str);
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                                bound->smin = domain->bound.smin;
                                bound->smax = domain->bound.smax;
                            }
                            *was_touched = true;
                            return;
                        }
                        else {
                            invalid_expr("Domain and expr type mismatch");
                            return;
                        }
                    }
                    else if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                        if(domain->attr_var.var != node->set_expr.right_value.variable_value.var) {
                            return;
                        }
                        if(domain->bound.value_type == VALUE_IL && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_INTEGER) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.integer_list_value.count != 0) {
                                    bound->ilmin = d64min(bound->ilmin, node->set_expr.left_value.integer_value);
                                    bound->ilmax = d64max(bound->ilmax, node->set_expr.left_value.integer_value);
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                                bound->ilmin = domain->bound.ilmin;
                                bound->ilmax = domain->bound.ilmax;
                            }
                            *was_touched = true;
                            return;
                        }
                        else if(domain->bound.value_type == VALUE_SL && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.string_list_value.count != 0) {
                                    bound->slmin = smin(bound->slmin, node->set_expr.left_value.string_value.str);
                                    bound->slmax = smax(bound->slmax, node->set_expr.left_value.string_value.str);
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                                bound->slmin = domain->bound.slmin;
                                bound->slmax = domain->bound.slmax;
                            }
                            *was_touched = true;
                            return;
                        }
                        else {
                            invalid_expr("Domain and expr type mismatch");
                            return;
                        }
                    }
                    else {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                default:
                    switch_default_error("Invalid set operation");
                    return;
            }
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_VARIABLE:
                    if(domain->attr_var.var != node->bool_expr.variable.var) {
                        return;
                    }
                    if(domain->bound.value_type != VALUE_B) {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                    if(is_reversed) {
                        bound->bmin = false;
                        if(!was_touched_value) {
                            bound->bmax = false;
                        }
                    }
                    else {
                        if(!was_touched_value) {
                            bound->bmin = true;
                        }
                        bound->bmax = true;
                    }
                    *was_touched = true;
                    return;
                case AST_BOOL_NOT:
                    get_variable_bound_inner(domain, node->bool_expr.unary.expr, bound, !is_reversed, was_touched);
                    return;
                case AST_BOOL_OR: {
                    struct value_bound lbound = copy_value_bound(bound);
                    bool l_touched = false;
                    struct value_bound rbound = copy_value_bound(bound);
                    bool r_touched = false;
                    get_variable_bound_inner(domain, node->bool_expr.binary.lhs, &lbound, is_reversed, &l_touched);
                    get_variable_bound_inner(domain, node->bool_expr.binary.rhs, &rbound, is_reversed, &r_touched);
                    if(l_touched == true && r_touched == true) {
                        *was_touched = true;
                        switch(bound->value_type) {
                            case VALUE_B:
                                bound->bmin = bound->bmin && lbound.bmin && rbound.bmax;
                                bound->bmax = bound->bmax || lbound.bmax || rbound.bmax;
                                break;
                            case VALUE_I:
                                bound->imin = d64min(bound->imin, d64min(lbound.imin, rbound.imin));
                                bound->imax = d64max(bound->imax, d64max(lbound.imax, rbound.imax));
                                break;
                            case VALUE_F:
                                bound->fmin = fmin(bound->fmin, fmin(lbound.fmin, rbound.fmin));
                                bound->fmax = fmax(bound->fmax, fmax(lbound.fmax, rbound.fmax));
                                break;
                            case VALUE_S:
                                bound->smin = smin(bound->smin, smin(lbound.smin, rbound.smin));
                                bound->smax = smax(bound->smax, smax(lbound.smax, rbound.smax));
                                break;
                            case VALUE_IL:
                                bound->ilmin = d64min(bound->ilmin, d64min(lbound.ilmin, rbound.ilmin));
                                bound->ilmax = d64max(bound->ilmax, d64max(lbound.ilmax, rbound.ilmax));
                                break;
                            case VALUE_SL:
                                bound->slmin = smin(bound->slmin, smin(lbound.slmin, rbound.slmin));
                                bound->slmax = smax(bound->slmax, smax(lbound.slmax, rbound.slmax));
                                break;
                            case VALUE_SEGMENTS:
                            case VALUE_FREQUENCY:
                            default:
                                break;
                        }
                    }
                    return;
                }
                case AST_BOOL_AND:
                    get_variable_bound_inner(domain, node->bool_expr.binary.lhs, bound, is_reversed, was_touched);
                    get_variable_bound_inner(domain, node->bool_expr.binary.rhs, bound, is_reversed, was_touched);
                    return;
                default:
                    switch_default_error("Invalid bool operation");
                    return;
            }
        case AST_TYPE_EQUALITY_EXPR:
            if(domain->attr_var.var != node->equality_expr.attr_var.var) {
                return;
            }
            if(!equality_value_matches(node->equality_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain and expr type mismatch");
                return;
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ:
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = domain->bound.imin;
                                bound->imax = domain->bound.imax;
                            }
                            else {
                                bound->imin = d64min(bound->imin, node->equality_expr.value.integer_value);
                                bound->imax = d64max(bound->imax, node->equality_expr.value.integer_value);
                            }
                            break;
                        case AST_EQUALITY_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = domain->bound.fmax;
                            }
                            else {
                                bound->fmin = fmin(bound->fmin, node->equality_expr.value.float_value);
                                bound->fmax = fmax(bound->fmax, node->equality_expr.value.float_value);
                            }
                            break;
                        case AST_EQUALITY_VALUE_STRING:
                            if(is_reversed) {
                                bound->smin = domain->bound.smin;
                                bound->smax = domain->bound.smax;
                            }
                            else {
                                bound->smin = smin(bound->smin, node->equality_expr.value.string_value.str);
                                bound->smax = smax(bound->smax, node->equality_expr.value.string_value.str);
                            }
                            break;
                        default:
                            switch_default_error("Invalid equality value type");
                            break;
                    }
                    break;
                case AST_EQUALITY_NE:
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = d64min(bound->imin, node->equality_expr.value.integer_value);
                                bound->imax = d64max(bound->imax, node->equality_expr.value.integer_value);
                            }
                            else {
                                bound->imin = domain->bound.imin;
                                bound->imax = domain->bound.imax;
                            }
                            break;
                        case AST_EQUALITY_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = fmin(bound->fmin, node->equality_expr.value.float_value);
                                bound->fmax = fmax(bound->fmax, node->equality_expr.value.float_value);
                            }
                            else {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = domain->bound.fmax;
                            }
                            break;
                        case AST_EQUALITY_VALUE_STRING:
                            if(is_reversed) {
                                bound->smin = smin(bound->smin, node->equality_expr.value.string_value.str);
                                bound->smax = smax(bound->smax, node->equality_expr.value.string_value.str);
                            }
                            else {
                                bound->smin = domain->bound.smin;
                                bound->smax = domain->bound.smax;
                            }
                            break;
                        default:
                            switch_default_error("Invalid equality value type");
                            break;
                    }
                    break;
                default:
                    switch_default_error("Invalid equality operation");
                    break;
            }
            *was_touched = true;
            return;
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            if(domain->attr_var.var != node->numeric_compare_expr.attr_var.var) {
                return;
            }
            if(!numeric_compare_value_matches(node->numeric_compare_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain and expr type mismatch");
                return;
            }
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT:
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = d64min(bound->imin, node->numeric_compare_expr.value.integer_value);
                                bound->imax = domain->bound.imax;
                            }
                            else {
                                bound->imin = domain->bound.imin;
                                bound->imax = d64max(bound->imax, node->numeric_compare_expr.value.integer_value - 1);
                            }
                            break;
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value);
                                bound->fmax = domain->bound.fmax;
                            }
                            else {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value - __DBL_EPSILON__);
                            }
                            break;
                        default:
                            switch_default_error("Invalid numeric compare value type");
                            break;
                    }
                    break;
                case AST_NUMERIC_COMPARE_LE:
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = d64min(bound->imin, node->numeric_compare_expr.value.integer_value + 1);
                                bound->imax = domain->bound.imax;
                            }
                            else {
                                bound->imin = domain->bound.imin;
                                bound->imax = d64max(bound->imax, node->numeric_compare_expr.value.integer_value);
                            }
                            break;
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value + __DBL_EPSILON__);
                                bound->fmax = domain->bound.fmax;
                            }
                            else {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value);
                            }
                            break;
                        default:
                            switch_default_error("Invalid numeric compare value type");
                            break;
                    }
                    break;
                case AST_NUMERIC_COMPARE_GT:
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = domain->bound.imin;
                                bound->imax = d64max(bound->imax, node->numeric_compare_expr.value.integer_value);
                            }
                            else {
                                bound->imin = d64min(bound->imin, node->numeric_compare_expr.value.integer_value + 1);
                                bound->imax = domain->bound.imax;
                            }
                            break;
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value);
                            }
                            else {
                                bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value + __DBL_EPSILON__);
                                bound->fmax = domain->bound.fmax;
                            }
                            break;
                        default:
                            switch_default_error("Invalid numeric compare value type");
                            break;
                    }
                    break;
                case AST_NUMERIC_COMPARE_GE:
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = domain->bound.imin;
                                bound->imax = d64max(bound->imax, node->numeric_compare_expr.value.integer_value - 1);
                            }
                            else {
                                bound->imin = d64min(bound->imin, node->numeric_compare_expr.value.integer_value);
                                bound->imax = domain->bound.imax;
                            }
                            break;
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = domain->bound.fmin;
                                bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value - __DBL_EPSILON__);
                            }
                            else {
                                bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value);
                                bound->fmax = domain->bound.fmax;
                            }
                            break;
                        default:
                            switch_default_error("Invalid numeric compare value type");
                            break;
                    }
                    break;
                default:
                    switch_default_error("Invalid numeric compare operation");
                    break;;
            }
            *was_touched = true;
            return;
        default:
            switch_default_error("Invalid expr type");
            return;
    }
}

struct value_bound get_variable_bound(const struct attr_domain* domain, const struct ast_node* node)
{
    bool was_touched = false;
    struct value_bound bound;
    switch(domain->bound.value_type) {
        case VALUE_B:
            bound.value_type = VALUE_B;
            bound.bmin = domain->bound.bmax;
            bound.bmax = domain->bound.bmin;
            break;
        case VALUE_I:
            bound.value_type = VALUE_I;
            bound.imin = domain->bound.imax;
            bound.imax = domain->bound.imin;
            break;
        case VALUE_F:
            bound.value_type = VALUE_F;
            bound.fmin = domain->bound.fmax;
            bound.fmax = domain->bound.fmin;
            break;
        case VALUE_S:
            bound.value_type = VALUE_S;
            bound.smin = domain->bound.smax;
            bound.smax = domain->bound.smin;
            break;
        case VALUE_IL:
            bound.value_type = VALUE_IL;
            bound.ilmin = domain->bound.ilmax;
            bound.ilmax = domain->bound.ilmin;
            break;
        case VALUE_SL:
            bound.value_type = VALUE_SL;
            bound.slmin = domain->bound.slmax;
            bound.slmax = domain->bound.slmin;
            break;
        case VALUE_SEGMENTS:
        case VALUE_FREQUENCY:
        default:
            fprintf(stderr, "Invalid domain type to get a bound\n");
            abort();
    }
    get_variable_bound_inner(domain, node, &bound, false, &was_touched);
    if(!was_touched) {
        switch(domain->bound.value_type) {
            case VALUE_B:
                bound.bmin = domain->bound.bmin;
                bound.bmax = domain->bound.bmax;
                break;
            case VALUE_I:
                bound.imin = domain->bound.imin;
                bound.imax = domain->bound.imax;
                break;
            case VALUE_F:
                bound.fmin = domain->bound.fmin;
                bound.fmax = domain->bound.fmax;
                break;
            case VALUE_S:
                bound.smin = domain->bound.smin;
                bound.smax = domain->bound.smax;
                break;
            case VALUE_IL:
                bound.ilmin = domain->bound.ilmin;
                bound.ilmax = domain->bound.ilmax;
                break;
            case VALUE_SL:
                bound.slmin = domain->bound.slmin;
                bound.slmax = domain->bound.slmax;
                break;
            case VALUE_SEGMENTS:
            case VALUE_FREQUENCY:
            default:
                fprintf(stderr, "Invalid domain type to get a bound\n");
                abort();
        }
    }
    return bound;
}

void assign_variable_id(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    betree_var_t variable_id = get_id_for_attr(config, node->special_expr.frequency.attr_var.attr);
                    node->special_expr.frequency.attr_var.var = variable_id;
                    betree_var_t now_id = get_id_for_attr(config, node->special_expr.frequency.now.attr);
                    node->special_expr.frequency.now.var = now_id;
                    return;
                }
                case AST_SPECIAL_SEGMENT: {
                    betree_var_t variable_id = get_id_for_attr(config, node->special_expr.segment.attr_var.attr);
                    node->special_expr.segment.attr_var.var = variable_id;
                    betree_var_t now_id = get_id_for_attr(config, node->special_expr.segment.now.attr);
                    node->special_expr.segment.now.var = now_id;
                    return;
                }
                case AST_SPECIAL_GEO: {
                    betree_var_t latitude_id = get_id_for_attr(config, node->special_expr.geo.latitude_var.attr);
                    node->special_expr.geo.latitude_var.var = latitude_id;
                    betree_var_t longitude_id = get_id_for_attr(config, node->special_expr.geo.longitude_var.attr);
                    node->special_expr.geo.longitude_var.var = longitude_id;
                    return;
                }
                case AST_SPECIAL_STRING: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->special_expr.string.attr_var.attr);
                    node->special_expr.string.attr_var.var = variable_id;
                    return;
                }
                default: {
                    switch_default_error("Invalid special expr type");
                    return;
                }
            }
            return;
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            betree_var_t variable_id
                = get_id_for_attr(config, node->numeric_compare_expr.attr_var.attr);
            node->numeric_compare_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->equality_expr.attr_var.attr);
            node->equality_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_NOT: {
                    assign_variable_id(config, node->bool_expr.unary.expr);
                    return;
                }
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    assign_variable_id(config, node->bool_expr.binary.lhs);
                    assign_variable_id(config, node->bool_expr.binary.rhs);
                    return;
                }
                case AST_BOOL_VARIABLE: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->bool_expr.variable.attr);
                    node->bool_expr.variable.var = variable_id;
                    return;
                }
                default:
                    switch_default_error("Invalid bool expr operation");
                    return;
            }
        }
        case(AST_TYPE_LIST_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->list_expr.attr_var.attr);
            node->list_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            switch(node->set_expr.left_value.value_type) {
                case AST_SET_LEFT_VALUE_INTEGER: {
                    break;
                }
                case AST_SET_LEFT_VALUE_STRING: {
                    break;
                }
                case AST_SET_LEFT_VALUE_VARIABLE: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->set_expr.left_value.variable_value.attr);
                    node->set_expr.left_value.variable_value.var = variable_id;
                    break;
                }
                default: {
                    switch_default_error("Invalid set left value type");
                }
            }
            switch(node->set_expr.right_value.value_type) {
                case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                    break;
                }
                case AST_SET_RIGHT_VALUE_STRING_LIST: {
                    break;
                }
                case AST_SET_RIGHT_VALUE_VARIABLE: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->set_expr.right_value.variable_value.attr);
                    node->set_expr.right_value.variable_value.var = variable_id;
                    break;
                }
                default: {
                    switch_default_error("Invalid set right value type");
                }
            }
            return;
        }
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

void assign_str_id(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    betree_str_t str_id
                        = get_id_for_string(config, node->special_expr.frequency.attr_var, node->special_expr.frequency.ns.string);
                    node->special_expr.frequency.ns.var = node->special_expr.frequency.attr_var.var;
                    node->special_expr.frequency.ns.str = str_id;
                    return;
                }
                case AST_SPECIAL_SEGMENT: {
                    return;
                }
                case AST_SPECIAL_GEO: {
                    return;
                }
                case AST_SPECIAL_STRING: {
                    return;
                }
                default:
                    switch_default_error("Invalid special expr type");
                    return;
            }
            return;
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                betree_str_t str_id
                    = get_id_for_string(config, node->equality_expr.attr_var, node->equality_expr.value.string_value.string);
                node->equality_expr.value.string_value.var = node->equality_expr.attr_var.var;
                node->equality_expr.value.string_value.str = str_id;
            }
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_NOT: {
                    assign_str_id(config, node->bool_expr.unary.expr);
                    return;
                }
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    assign_str_id(config, node->bool_expr.binary.lhs);
                    assign_str_id(config, node->bool_expr.binary.rhs);
                    return;
                }
                case AST_BOOL_VARIABLE: {
                    return;
                }
                default:
                    switch_default_error("Invalid bool expr operation");
                    return;
            }
        }
        case(AST_TYPE_LIST_EXPR): {
            if(node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->list_expr.value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(
                        config, node->list_expr.attr_var, node->list_expr.value.string_list_value.strings[i].string);
                    node->list_expr.value.string_list_value.strings[i].var = node->list_expr.attr_var.var;
                    node->list_expr.value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                betree_str_t str_id
                    = get_id_for_string(config, node->set_expr.right_value.variable_value, node->set_expr.left_value.string_value.string);
                node->set_expr.left_value.string_value.var = node->set_expr.right_value.variable_value.var;
                node->set_expr.left_value.string_value.str = str_id;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->set_expr.right_value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(
                        config, node->set_expr.left_value.variable_value, node->set_expr.right_value.string_list_value.strings[i].string);
                    node->set_expr.right_value.string_list_value.strings[i].var = node->set_expr.left_value.variable_value.var;
                    node->set_expr.right_value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

bool eq_numeric_compare_value(struct numeric_compare_value a, struct numeric_compare_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
            return feq(a.float_value, b.float_value);
        default:
            switch_default_error("Invalid numeric compare value type");
            return false;
    }
}

bool eq_equality_value(struct equality_value a, struct equality_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_EQUALITY_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_EQUALITY_VALUE_FLOAT:
            return feq(a.float_value, b.float_value);
        case AST_EQUALITY_VALUE_STRING:
            return a.string_value.var == b.string_value.var && a.string_value.str == b.string_value.str;
        default:
            switch_default_error("Invalid equality value type");
            return false;
    }
}

bool eq_bool_expr(struct ast_bool_expr a, struct ast_bool_expr b)
{
    if(a.op != b.op) {
        return false;
    }
    switch(a.op) {
        case AST_BOOL_OR:
        case AST_BOOL_AND:
            return eq_expr(a.binary.lhs, b.binary.lhs) && eq_expr(a.binary.rhs, b.binary.rhs);
        case AST_BOOL_NOT:
            return eq_expr(a.unary.expr, b.unary.expr);
        case AST_BOOL_VARIABLE:
            return a.variable.var == b.variable.var;
        default:
            switch_default_error("Invalid bool expr op");
            return false;
    }
}

bool eq_set_left_value(struct set_left_value a, struct set_left_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_SET_LEFT_VALUE_STRING:
            return a.string_value.var == b.string_value.var && a.string_value.str == b.string_value.str;
        case AST_SET_LEFT_VALUE_VARIABLE:
            return a.variable_value.var == b.variable_value.var;
        default:
            switch_default_error("Invalid left value type");
            return false;
    }
}

bool eq_integer_list(struct integer_list_value a, struct integer_list_value b)
{
    if(a.count != b.count) {
        return false;
    }
    for(size_t i = 0; i < a.count; i++) {
        if(a.integers[i] != b.integers[i]) {
            return false;
        }
    }
    return true;
}

bool eq_string_list(struct string_list_value a, struct string_list_value b)
{
    if(a.count != b.count) {
        return false;
    }
    for(size_t i = 0; i < a.count; i++) {
        if(a.strings[i].var == b.strings[i].var && a.strings[i].str != b.strings[i].str) {
            return false;
        }
    }
    return true;
}

bool eq_set_right_value(struct set_right_value a, struct set_right_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_SET_RIGHT_VALUE_INTEGER_LIST:
            return eq_integer_list(a.integer_list_value, b.integer_list_value);
        case AST_SET_RIGHT_VALUE_STRING_LIST:
            return eq_string_list(a.string_list_value, b.string_list_value);
        case AST_SET_RIGHT_VALUE_VARIABLE:
            return a.variable_value.var == b.variable_value.var;
        default:
            switch_default_error("Invalid right value type");
            return false;
    }
}

bool eq_set_expr(struct ast_set_expr a, struct ast_set_expr b)
{
    if(a.op != b.op) {
        return false;
    }
    return eq_set_left_value(a.left_value, b.left_value) && eq_set_right_value(a.right_value, b.right_value);
}

bool eq_list_value(struct list_value a, struct list_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST:
            return eq_integer_list(a.integer_list_value, b.integer_list_value);
        case AST_LIST_VALUE_STRING_LIST:
            return eq_string_list(a.string_list_value, b.string_list_value);
        default:
            switch_default_error("Invalid list value type");
            return false;
    }
}

bool eq_list_expr(struct ast_list_expr a, struct ast_list_expr b)
{
    if(a.op != b.op) {
        return false;
    }
    return a.attr_var.var == b.attr_var.var && eq_list_value(a.value, b.value);
}

bool eq_geo_value(struct special_geo_value a, struct special_geo_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_SPECIAL_GEO_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_SPECIAL_GEO_VALUE_FLOAT:
            return feq(a.float_value, b.float_value);
        default:
            switch_default_error("Invalid geo value type");
            return false;
    }
}

bool eq_special_expr(struct ast_special_expr a, struct ast_special_expr b)
{
    if(a.type != b.type) {
        return false;
    }
    switch(a.type) {
        case AST_SPECIAL_FREQUENCY:
            return
                a.frequency.attr_var.var == b.frequency.attr_var.var &&
                a.frequency.length == b.frequency.length &&
                a.frequency.ns.var == b.frequency.ns.var &&
                a.frequency.ns.str == b.frequency.ns.str &&
                a.frequency.op == b.frequency.op &&
                a.frequency.type == b.frequency.type &&
                a.frequency.value == b.frequency.value;
        case AST_SPECIAL_SEGMENT:
            return
                a.segment.attr_var.var == b.segment.attr_var.var &&
                a.segment.has_variable == b.segment.has_variable &&
                a.segment.op == b.segment.op &&
                a.segment.seconds == b.segment.seconds &&
                a.segment.segment_id == b.segment.segment_id;
        case AST_SPECIAL_GEO:
            return
                a.geo.has_radius == b.geo.has_radius &&
                eq_geo_value(a.geo.latitude, b.geo.latitude) &&
                eq_geo_value(a.geo.longitude, b.geo.longitude) &&
                a.geo.op == b.geo.op &&
                eq_geo_value(a.geo.radius, b.geo.radius);
        case AST_SPECIAL_STRING:
            return
                a.string.attr_var.var == b.string.attr_var.var &&
                a.string.op == b.string.op &&
                strcmp(a.string.pattern, b.string.pattern) == 0;
        default:
            switch_default_error("Invalid special expr type");
            return false;
    }
}

bool eq_expr(const struct ast_node* a, const struct ast_node* b)
{
    if(a == NULL || b == NULL) {
        return false;
    }
    if(a->type != b->type) {
        return false;
    }
    switch(a->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            return
                a->numeric_compare_expr.attr_var.var == b->numeric_compare_expr.attr_var.var &&
                a->numeric_compare_expr.op == b->numeric_compare_expr.op &&
                eq_numeric_compare_value(a->numeric_compare_expr.value, b->numeric_compare_expr.value);
        }
        case AST_TYPE_EQUALITY_EXPR: {
            return
                a->equality_expr.attr_var.var == b->equality_expr.attr_var.var &&
                a->equality_expr.op == b->equality_expr.op &&
                eq_equality_value(a->equality_expr.value, b->equality_expr.value);
        }
        case AST_TYPE_BOOL_EXPR: {
            return eq_bool_expr(a->bool_expr, b->bool_expr);
        }
        case AST_TYPE_SET_EXPR: {
            return eq_set_expr(a->set_expr, b->set_expr);
        }
        case AST_TYPE_LIST_EXPR: {
            return eq_list_expr(a->list_expr, b->list_expr);
        }
        case AST_TYPE_SPECIAL_EXPR: {
            return eq_special_expr(a->special_expr, b->special_expr);
        }
        default:
            switch_default_error("Invalid node type");
            return false;
    }
}

bool fast_eq_expr(const struct ast_node* a, const struct ast_node* b)
{
    if(a == NULL || b == NULL) {
        return false;
    }
    if(a->type != b->type) {
        return false;
    }
    if(a->type == AST_TYPE_BOOL_EXPR) {
        if(a->bool_expr.op == AST_BOOL_NOT) {
            return a->bool_expr.unary.expr->id == b->bool_expr.unary.expr->id;
        }
        if(a->bool_expr.op == AST_BOOL_AND || a->bool_expr.op == AST_BOOL_OR) {
            return a->bool_expr.binary.lhs->id == b->bool_expr.binary.lhs->id && a->bool_expr.binary.rhs->id == b->bool_expr.binary.rhs->id;
        }
    }
    return a->id == b->id;
}

void assign_pred_id(struct config* config, struct ast_node* node)
{
    assign_pred(config->pred_map, node);
}

void sort_lists(struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR: 
        case AST_TYPE_EQUALITY_EXPR:
            return;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    sort_lists(node->bool_expr.binary.lhs);
                    sort_lists(node->bool_expr.binary.rhs);
                    return;
                case AST_BOOL_NOT:
                    return sort_lists(node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return;
                default:
                    switch_default_error("Invalid bool expr op");
                    return;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                switch(node->set_expr.right_value.value_type) {
                    case AST_SET_RIGHT_VALUE_INTEGER_LIST:
                        qsort(node->set_expr.right_value.integer_list_value.integers, 
                          node->set_expr.right_value.integer_list_value.count, 
                          sizeof(*node->set_expr.right_value.integer_list_value.integers),
                          icmpfunc);
                        return;
                    case AST_SET_RIGHT_VALUE_STRING_LIST:
                        qsort(node->set_expr.right_value.string_list_value.strings, 
                          node->set_expr.right_value.string_list_value.count, 
                          sizeof(*node->set_expr.right_value.string_list_value.strings),
                          scmpfunc);
                        return;
                    case AST_SET_RIGHT_VALUE_VARIABLE:
                    default:
                        fprintf(stderr, "Invalid set expr");
                        abort();
                        return;
                }
            }
            return;
        case AST_TYPE_LIST_EXPR:
            switch(node->list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST:
                    qsort(node->list_expr.value.integer_list_value.integers, 
                      node->list_expr.value.integer_list_value.count, 
                      sizeof(*node->list_expr.value.integer_list_value.integers),
                      icmpfunc);
                    return;
                case AST_LIST_VALUE_STRING_LIST:
                    qsort(node->list_expr.value.string_list_value.strings, 
                      node->list_expr.value.string_list_value.count, 
                      sizeof(*node->list_expr.value.string_list_value.strings),
                      scmpfunc);
                    return;
                default:
                    switch_default_error("Invalid list value type");
                    return;
            }
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                case AST_SPECIAL_SEGMENT:
                case AST_SPECIAL_GEO:
                case AST_SPECIAL_STRING:
                    return;
                default:
                    switch_default_error("Invalid special expr type");
                    return;
            }
        default:
            switch_default_error("Invalid node type");
            return;
    }
}

bool var_exists(const struct config* config, const char* attr)
{
    for(size_t i = 0; i < config->attr_to_id_count; i++) {
        if(strcmp(attr, config->attr_to_ids[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool all_variables_in_config(const struct config* config, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            return var_exists(config, node->numeric_compare_expr.attr_var.attr);
        case AST_TYPE_EQUALITY_EXPR:
            return var_exists(config, node->equality_expr.attr_var.attr);
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    return all_variables_in_config(config, node->bool_expr.binary.lhs) && all_variables_in_config(config, node->bool_expr.binary.rhs);
                case AST_BOOL_NOT:
                    return all_variables_in_config(config, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return var_exists(config, node->bool_expr.variable.attr);
                default:
                    switch_default_error("Invalid bool expr op");
                    return false;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                return var_exists(config, node->set_expr.left_value.variable_value.attr);
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                return var_exists(config, node->set_expr.right_value.variable_value.attr);
            }
            fprintf(stderr, "Invalid set expr");
            abort();
            return false;
        case AST_TYPE_LIST_EXPR:
            return var_exists(config, node->list_expr.attr_var.attr);
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    return var_exists(config, node->special_expr.frequency.attr_var.attr);
                case AST_SPECIAL_SEGMENT:
                    return var_exists(config, node->special_expr.segment.attr_var.attr);
                case AST_SPECIAL_GEO:
                    return true;
                case AST_SPECIAL_STRING:
                    return var_exists(config, node->special_expr.string.attr_var.attr);
                default:
                    switch_default_error("Invalid special expr type");
                    return false;
            }
        default:
            switch_default_error("Invalid node type");
            return false;
    }
}

static size_t get_attr_string_bound(const struct config* config, const char* attr)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        if(strcmp(attr, config->attr_domains[i]->attr_var.attr) == 0) {
            betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_TYPE_MISMATCH, 
              config->attr_domains[i]->bound.value_type == VALUE_S
              || config->attr_domains[i]->bound.value_type == VALUE_SL);
            if(config->attr_domains[i]->bound.is_string_bounded == false) {
                return (size_t)-1;
            }
            else {
                return config->attr_domains[i]->bound.smax;
            }
        }
    }
    return (size_t)-1;
}

static struct string_map* get_string_map_for_attr(const struct config* config, const char* attr)
{
    for(size_t i = 0; i < config->string_map_count; i++) {
        struct string_map* string_map = &config->string_maps[i];
        if(strcmp(attr, string_map->attr_var.attr) == 0) {
            return string_map;
        }
    }
    return NULL;
}

static bool str_valid(const struct config* config, const char* attr, const char* string)
{
    size_t bound = get_attr_string_bound(config, attr);
    if(bound == (size_t)-1) {
        return true;
    }
    struct string_map* string_map = get_string_map_for_attr(config, attr);
    size_t space_left = string_map == NULL ? bound : bound - string_map->string_value_count + 1;
    if(string_map != NULL) {
        for(size_t j = 0; j < string_map->string_value_count; j++) {
            if(strcmp(string, string_map->string_values[j]) == 0) {
                return true;
            }
        }
    }
    bool within_bound = space_left > 0;
    if(within_bound == false) {
        fprintf(stderr, "For attr '%s', string '%s' could not fit within the bound %zu\n", attr, string, bound);
    }
    return within_bound;
}

static bool strs_valid(const struct config* config, const char* attr, const struct string_list_value* strings)
{
    size_t bound = get_attr_string_bound(config, attr);
    if(bound == (size_t)-1) {
        return true;
    }
    struct string_map* string_map = get_string_map_for_attr(config, attr);
    size_t space_left = string_map == NULL ? bound : bound - string_map->string_value_count + 1;
    size_t found = 0;
    if(string_map != NULL) {
        for(size_t i = 0; i < strings->count; i++) {
            const char* string = strings->strings[i].string;
            for(size_t j = 0; j < string_map->string_value_count; j++) {
                if(strcmp(string, string_map->string_values[j]) == 0) {
                    found++;
                    break;
                }
            }
        }
    }
    size_t needed = strings->count - found;
    bool within_bound = space_left >= needed;
    if(within_bound == false) {
        fprintf(stderr, "For attr '%s', not all strings could not fit within the bound %zu\n", attr, bound);
    }
    return within_bound;
}

bool all_bounded_strings_valid(const struct config* config, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            return true;
        case AST_TYPE_EQUALITY_EXPR:
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                return str_valid(config, node->equality_expr.attr_var.attr, node->equality_expr.value.string_value.string);
            }
            return true;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    bool lhs = all_bounded_strings_valid(config, node->bool_expr.binary.lhs);
                    if(lhs == false) {
                        return false;
                    }
                    return all_bounded_strings_valid(config, node->bool_expr.binary.rhs);
                }
                case AST_BOOL_NOT:
                    return all_bounded_strings_valid(config, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return true;
                default:
                    switch_default_error("Invalid bool expr op");
                    return false;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                return strs_valid(config, node->set_expr.left_value.variable_value.attr, &node->set_expr.right_value.string_list_value);
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                return str_valid(config, node->set_expr.right_value.variable_value.attr, node->set_expr.left_value.string_value.string);
            }
            return true;
        case AST_TYPE_LIST_EXPR:
            if(node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                return strs_valid(config, node->list_expr.attr_var.attr, &node->list_expr.value.string_list_value);
            }
            return true;
        case AST_TYPE_SPECIAL_EXPR:
            return true;
        default:
            switch_default_error("Invalid node type");
            return false;
    }
}

