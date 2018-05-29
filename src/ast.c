#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
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

struct ast_node* ast_bool_expr_binary_create(
    enum ast_bool_e op, struct ast_node* lhs, struct ast_node* rhs)
{
    struct ast_node* node = ast_bool_expr_create(op);
    node->bool_expr.binary.lhs = lhs;
    node->bool_expr.binary.rhs = rhs;
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
        .radius = radius };
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
            free((char*)special_expr.frequency.ns.string);
            break;
        case AST_SPECIAL_SEGMENT:
            free_attr_var(special_expr.segment.attr_var);
            break;
        case AST_SPECIAL_GEO:
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

bool integer_in_integer_list(int64_t integer, struct integer_list_value list)
{
    for(size_t i = 0; i < list.count; i++) {
        if(list.integers[i] == integer) {
            return true;
        }
    }
    return false;
}

bool string_in_string_list(struct string_value string, struct string_list_value list)
{
    for(size_t i = 0; i < list.count; i++) {
        if(list.strings[i].str == string.str) {
            return true;
        }
    }
    return false;
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

struct string_value frequency_type_to_string(struct config* config, enum frequency_type_e type)
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
    betree_str_t str = get_id_for_string(config, string);
    struct string_value ret = { .string = string, .str = str };
    return ret;
}

bool match_special_expr(
    struct config* config, const struct event* event, const struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY: {
            switch(special_expr.frequency.op) {
                case AST_SPECIAL_WITHINFREQUENCYCAP: {
                    int64_t now;
                    enum variable_state_e state = get_integer_attr(config, event, "now", &now);
                    betree_assert(state != VARIABLE_MISSING, "Attribute 'now' is not defined");
                    if(state == VARIABLE_UNDEFINED) {
                        return false;
                    }
                    struct frequency_caps_list caps;
                    enum variable_state_e caps_state = get_frequency_attr(config, event, &caps);
                    betree_assert(caps_state != VARIABLE_MISSING, "Attribute is not defined");
                    if(caps_state == VARIABLE_UNDEFINED) {
                        return false;
                    }
                    else {
                        uint32_t id;
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
                        return within_frequency_caps(&caps,
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
            enum variable_state_e now_state = get_integer_attr(config, event, "now", &now);
            betree_assert(now_state != VARIABLE_MISSING, "Attribute 'now' is not defined");
            struct segments_list segments;
            const char* segments_attr = special_expr.segment.has_variable
                ? special_expr.segment.attr_var.attr
                : "segments_with_timestamp";
            enum variable_state_e segments_state
                = get_segments_attr(config, event, segments_attr, &segments);
            betree_assert(segments_state != VARIABLE_MISSING, "Attribute is not defined");
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
                        = get_float_attr(config, event, "latitude", &latitude_var);
                    enum variable_state_e longitude_var_state
                        = get_float_attr(config, event, "longitude", &longitude_var);
                    betree_assert(latitude_var_state != VARIABLE_MISSING,
                        "Attribute 'latitude' is not defined");
                    betree_assert(longitude_var_state != VARIABLE_MISSING,
                        "Attribute 'longitude' is not defined");
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
                = get_string_attr(config, event, special_expr.string.attr_var.attr, &value);
            betree_assert(state != VARIABLE_MISSING, "Attribute is not defined");
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

bool match_list_expr(
    const struct config* config, const struct event* event, const struct ast_list_expr list_expr)
{
    struct value variable;
    enum variable_state_e state = get_variable(config, list_expr.attr_var.var, event, &variable);
    betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(list_value_matches(list_expr.value.value_type, variable.value_type),
        "List value types do not match");
    switch(list_expr.op) {
        case AST_LIST_ONE_OF:
        case AST_LIST_NONE_OF: {
            bool result = false;
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: {
                    for(size_t i = 0; i < variable.ilvalue.count; i++) {
                        int64_t left = variable.ilvalue.integers[i];
                        for(size_t j = 0; j < list_expr.value.integer_list_value.count; j++) {
                            int64_t right = list_expr.value.integer_list_value.integers[j];
                            if(left == right) {
                                result = true;
                                break;
                            }
                        }
                        if(result == true) {
                            break;
                        }
                    }
                    break;
                }
                case AST_LIST_VALUE_STRING_LIST: {
                    for(size_t i = 0; i < variable.slvalue.count; i++) {
                        betree_str_t left = variable.slvalue.strings[i].str;
                        for(size_t j = 0; j < list_expr.value.string_list_value.count; j++) {
                            betree_str_t right = list_expr.value.string_list_value.strings[j].str;
                            if(left == right) {
                                result = true;
                                break;
                            }
                        }
                        if(result == true) {
                            break;
                        }
                    }
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
            size_t count = 0, target_count = 0;
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: {
                    target_count = list_expr.value.integer_list_value.count;
                    for(size_t i = 0; i < target_count; i++) {
                        int64_t right = list_expr.value.integer_list_value.integers[i];
                        for(size_t j = 0; j < variable.ilvalue.count; j++) {
                            int64_t left = variable.ilvalue.integers[j];
                            if(left == right) {
                                count++;
                                break;
                            }
                        }
                    }
                    break;
                }
                case AST_LIST_VALUE_STRING_LIST: {
                    target_count = list_expr.value.string_list_value.count;
                    for(size_t i = 0; i < target_count; i++) {
                        betree_str_t right = list_expr.value.string_list_value.strings[i].str;
                        for(size_t j = 0; j < variable.slvalue.count; j++) {
                            betree_str_t left = variable.slvalue.strings[j].str;
                            if(left == right) {
                                count++;
                                break;
                            }
                        }
                    }
                    break;
                }
                default: {
                    switch_default_error("Invalid list value type");
                    return false;
                }
            }
            return count == target_count;
        }
        default: {
            switch_default_error("Invalid list operation");
            return false;
        }
    }
}

bool match_set_expr(
    const struct config* config, const struct event* event, const struct ast_set_expr set_expr)
{
    struct set_left_value left = set_expr.left_value;
    struct set_right_value right = set_expr.right_value;
    bool is_in;
    if(left.value_type == AST_SET_LEFT_VALUE_INTEGER
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct integer_list_value variable;
        enum variable_state_e state
            = get_integer_list_var(config, right.variable_value.var, event, &variable);
        betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = integer_in_integer_list(left.integer_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_STRING
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct string_list_value variable;
        enum variable_state_e state
            = get_string_list_var(config, right.variable_value.var, event, &variable);
        betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = string_in_string_list(left.string_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
        int64_t variable;
        enum variable_state_e state
            = get_integer_var(config, left.variable_value.var, event, &variable);
        betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
        if(state == VARIABLE_UNDEFINED) {
            return false;
        }
        is_in = integer_in_integer_list(variable, right.integer_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
        struct string_value variable;
        enum variable_state_e state
            = get_string_var(config, left.variable_value.var, event, &variable);
        betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
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
    const struct event* event,
    const struct ast_numeric_compare_expr numeric_compare_expr)
{
    struct value variable;
    enum variable_state_e state
        = get_variable(config, numeric_compare_expr.attr_var.var, event, &variable);
    betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(
        numeric_compare_value_matches(numeric_compare_expr.value.value_type, variable.value_type),
        "Numeric compare value types do not match");
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
    const struct event* event,
    const struct ast_equality_expr equality_expr)
{
    struct value variable;
    enum variable_state_e state
        = get_variable(config, equality_expr.attr_var.var, event, &variable);
    betree_assert(state != VARIABLE_MISSING, "Variable is not defined");
    if(state == VARIABLE_UNDEFINED) {
        return false;
    }
    betree_assert(equality_value_matches(equality_expr.value.value_type, variable.value_type),
        "Equality value types do not match");
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

static bool match_node_inner(struct config* config, const struct event* event, const struct ast_node* node, struct memoize* memoize, struct report* report, bool is_top_level);

bool match_bool_expr(
    struct config* config, const struct event* event, const struct ast_bool_expr bool_expr, struct memoize* memoize, struct report* report)
{
    switch(bool_expr.op) {
        case AST_BOOL_AND: {
            bool lhs = match_node_inner(config, event, bool_expr.binary.lhs, memoize, report, false);
            if(lhs == false) {
                return false;
            }
            bool rhs = match_node_inner(config, event, bool_expr.binary.rhs, memoize, report, false);
            return rhs;
        }
        case AST_BOOL_OR: {
            bool lhs = match_node_inner(config, event, bool_expr.binary.lhs, memoize, report, false);
            if(lhs == true) {
                return true;
            }
            bool rhs = match_node_inner(config, event, bool_expr.binary.rhs, memoize, report, false);
            return rhs;
        }
        case AST_BOOL_NOT: {
            bool result = match_node_inner(config, event, bool_expr.unary.expr, memoize, report, false);
            return !result;
        }
        case AST_BOOL_VARIABLE: {
            bool value;
            enum variable_state_e state
                = get_bool_var(config, bool_expr.variable.var, event, &value);
            betree_assert(state != VARIABLE_MISSING, "Variable is missing");
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

void report_memoized(struct report* report, bool is_top_level)
{
    if(report != NULL) {
        if(is_top_level) {
            report->expressions_memoized++;
        }
        report->sub_expressions_memoized++;
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

static bool match_node_inner(struct config* config, const struct event* event, const struct ast_node* node, struct memoize* memoize, struct report* report, bool is_top_level)
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
            report_memoized(report, is_top_level);
            return true;
        }
        if(test_bit(memoize->fail, node->id)) {
            report_memoized(report, is_top_level);
            return false;
        }
    }
    bool result;
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            result = match_special_expr(config, event, node->special_expr);
            break;
        }
        case AST_TYPE_BOOL_EXPR: {
            result = match_bool_expr(config, event, node->bool_expr, memoize, report);
            break;
        }
        case AST_TYPE_LIST_EXPR: {
            result = match_list_expr(config, event, node->list_expr);
            break;
        }
        case AST_TYPE_SET_EXPR: {
            result = match_set_expr(config, event, node->set_expr);
            break;
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            result = match_numeric_compare_expr(config, event, node->numeric_compare_expr);
            break;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            result = match_equality_expr(config, event, node->equality_expr);
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

bool match_node(struct config* config, const struct event* event, const struct ast_node* node, struct memoize* memoize, struct report* report)
{
    return match_node_inner(config, event, node, memoize, report, true);
}

void get_variable_bound(
    const struct attr_domain* domain, const struct ast_node* node, struct value_bound* bound)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            return;
        }
        case AST_TYPE_LIST_EXPR: {
            return;
        }
        case AST_TYPE_SET_EXPR: {
            return;
        }
        case AST_TYPE_BOOL_EXPR: {
            switch(node->bool_expr.op) {
                case AST_BOOL_VARIABLE: {
                    return;
                }
                case AST_BOOL_NOT: {
                    get_variable_bound(domain, node->bool_expr.unary.expr, bound);
                    return;
                }
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    get_variable_bound(domain, node->bool_expr.binary.lhs, bound);
                    get_variable_bound(domain, node->bool_expr.binary.rhs, bound);
                    return;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return;
                }
            }
        }
        case AST_TYPE_EQUALITY_EXPR: {
            if(domain->attr_var.var != node->equality_expr.attr_var.var) {
                return;
            }
            if(domain->bound.value_type != bound->value_type
                || !equality_value_matches(
                       node->equality_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
                return;
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bound->imin = d64min(bound->imin, node->equality_expr.value.integer_value);
                            bound->imax = d64max(bound->imax, node->equality_expr.value.integer_value);
                            return;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->equality_expr.value.float_value);
                            bound->fmax = fmax(bound->fmax, node->equality_expr.value.float_value);
                            return;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            invalid_expr("Trying to get the bound of a string value");
                            return;
                        }
                        default: {
                            switch_default_error("Invalid equality value type");
                            return;
                        }
                    }
                }
                case AST_EQUALITY_NE: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            invalid_expr("Trying to get the bound of a string value");
                            return;
                        }
                        default: {
                            switch_default_error("Invalid equality value type");
                            return;
                        }
                    }
                }
                default: {
                    switch_default_error("Invalid equality operation");
                    return;
                }
            }
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            if(domain->attr_var.var != node->numeric_compare_expr.attr_var.var) {
                return;
            }
            if(domain->bound.value_type != bound->value_type
                || !numeric_compare_value_matches(
                       node->numeric_compare_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
                return;
            }
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = d64max(
                                bound->imax, node->numeric_compare_expr.value.integer_value - 1);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax,
                                node->numeric_compare_expr.value.float_value - __DBL_EPSILON__);
                            return;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_LE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax
                                = d64max(bound->imax, node->numeric_compare_expr.value.integer_value);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax
                                = fmax(bound->fmax, node->numeric_compare_expr.value.float_value);
                            return;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = d64min(
                                bound->imin, node->numeric_compare_expr.value.integer_value + 1);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin,
                                node->numeric_compare_expr.value.float_value + __DBL_EPSILON__);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin
                                = d64min(bound->imin, node->numeric_compare_expr.value.integer_value);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin
                                = fmin(bound->fmin, node->numeric_compare_expr.value.float_value);
                            bound->fmax = domain->bound.fmax;
                            return;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return;
                        }
                    }
                }
                default: {
                    switch_default_error("Invalid numeric compare operation");
                    return;
                }
            }
        }
        default: {
            switch_default_error("Invalid expr type");
            return;
        }
    }
}

void assign_variable_id(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->special_expr.frequency.attr_var.attr);
                    node->special_expr.frequency.attr_var.var = variable_id;
                    return;
                }
                case AST_SPECIAL_SEGMENT: {
                    betree_var_t variable_id
                        = get_id_for_attr(config, node->special_expr.segment.attr_var.attr);
                    node->special_expr.segment.attr_var.var = variable_id;
                    return;
                }
                case AST_SPECIAL_GEO: {
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
                        = get_id_for_string(config, node->special_expr.frequency.ns.string);
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
                    = get_id_for_string(config, node->equality_expr.value.string_value.string);
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
                        config, node->list_expr.value.string_list_value.strings[i].string);
                    node->list_expr.value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                betree_str_t str_id
                    = get_id_for_string(config, node->set_expr.left_value.string_value.string);
                node->set_expr.left_value.string_value.str = str_id;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->set_expr.right_value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(
                        config, node->set_expr.right_value.string_list_value.strings[i].string);
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
            return a.string_value.str == b.string_value.str;
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
            return a.string_value.str == b.string_value.str;
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
        if(a.strings[i].str != b.strings[i].str) {
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

void assign_pred_id(struct config* config, struct ast_node* node)
{
    assign_pred(config->pred_map, node);
}

