#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "betree.h"
#include "functions.h"
#include "utils.h"

struct ast_node* ast_node_create()
{
    struct ast_node* node = calloc(1, sizeof(*node));
    if(node == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    return node;
}

struct ast_node* ast_numeric_compare_expr_create(const enum ast_numeric_compare_e op, const char* name, struct numeric_compare_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_NUMERIC_COMPARE_EXPR;
    node->numeric_compare_expr.op = op;
    node->numeric_compare_expr.name = strdup(name);
    node->numeric_compare_expr.variable_id = -1;
    node->numeric_compare_expr.value = value;
    return node;
}

struct ast_node* ast_equality_expr_create(const enum ast_equality_e op, const char* name, struct equality_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_EQUALITY_EXPR;
    node->equality_expr.op = op;
    node->equality_expr.name = strdup(name);
    node->equality_expr.variable_id = -1;
    node->equality_expr.value = value;
    return node;
}

struct ast_node* ast_bool_expr_create(const enum ast_bool_e op, const char* name)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_BOOL_EXPR;
    node->bool_expr.op = op;
    node->bool_expr.name = strdup(name);
    node->bool_expr.variable_id = -1;
    return node;
}

struct ast_node* ast_combi_expr_create(const enum ast_combi_e op, const struct ast_node* lhs, const struct ast_node* rhs)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_COMBI_EXPR;
    node->combi_expr.op = op;
    node->combi_expr.lhs = lhs;
    node->combi_expr.rhs = rhs;
    return node;
}

struct ast_node* ast_set_expr_create(const enum ast_set_e op, struct set_left_value left_value, struct set_right_value right_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_SET_EXPR;
    node->set_expr.op = op;
    node->set_expr.left_value = left_value;
    node->set_expr.right_value = right_value;
    return node;
}

struct ast_node* ast_list_expr_create(const enum ast_list_e op, const char* name, struct list_value list_value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_LIST_EXPR;
    node->list_expr.op = op;
    node->list_expr.name = strdup(name);
    node->list_expr.variable_id = -1;
    node->list_expr.value = list_value;
    return node;
}

struct ast_node* ast_special_expr_create()
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_SPECIAL_EXPR;
    return node;
}

struct ast_node* ast_special_frequency_create(const enum ast_special_frequency_e op, enum ast_special_frequency_type_e type, struct string_value ns, int64_t value, size_t length)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_frequency frequency = {
        .op = op,
        .type = type,
        .ns = ns,
        .value = value,
        .length = length
    };
    node->special_expr.type = AST_SPECIAL_FREQUENCY;
    node->special_expr.frequency = frequency;
    return node;
}

struct ast_node* ast_special_segment_create(const enum ast_special_segment_e op, const char* name, betree_seg_t segment_id, int64_t seconds)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_segment segment = {
        .op = op,
        .segment_id = segment_id,
        .seconds = seconds
    };
    if(name == NULL) {
        segment.has_variable = false;
        segment.name = NULL;
    }
    else {
        segment.has_variable = true;
        segment.name = strdup(name);
    }
    segment.variable_id = -1;
    node->special_expr.type = AST_SPECIAL_SEGMENT;
    node->special_expr.segment = segment;
    return node;
}

struct ast_node* ast_special_geo_create(const enum ast_special_geo_e op, struct special_geo_value latitude, struct special_geo_value longitude, bool has_radius, struct special_geo_value radius)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_geo geo = {
        .op = op,
        .latitude = latitude,
        .longitude = longitude,
        .has_radius = has_radius,
        .radius = radius
    };
    node->special_expr.type = AST_SPECIAL_GEO;
    node->special_expr.geo = geo;
    return node;
}

struct ast_node* ast_special_string_create(const enum ast_special_string_e op, const char* name, const char* pattern)
{
    struct ast_node* node = ast_special_expr_create();
    struct ast_special_string string = {
        .op = op,
        .name = strdup(name),
        .variable_id = -1,
        .pattern = strdup(pattern)
    };
    node->special_expr.type = AST_SPECIAL_STRING;
    node->special_expr.string = string;
    return node;
}

void free_special_expr(struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY:
            free((char*)special_expr.frequency.ns.string);
            break;
        case AST_SPECIAL_SEGMENT:
            free((char*)special_expr.segment.name);
            break;
        case AST_SPECIAL_GEO:
            break;
        case AST_SPECIAL_STRING:
            free((char*)special_expr.string.name);
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
            free((char*)set_expr.left_value.variable_value.name);
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
            free((char*)set_expr.right_value.variable_value.name);
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
    free((char*)list_expr.name);
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
            free((char*)node->numeric_compare_expr.name);
            break;
        case AST_TYPE_EQUALITY_EXPR:
            free((char*)node->equality_expr.name);
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                free((char*)node->equality_expr.value.string_value.string);
            }
            break;
        case AST_TYPE_BOOL_EXPR:
            free((char*)node->bool_expr.name);
            break;
        case AST_TYPE_SET_EXPR:
            free_set_expr(node->set_expr);
            break;
        case AST_TYPE_LIST_EXPR:
            free_list_expr(node->list_expr);
            break;
        case AST_TYPE_COMBI_EXPR:
            free_ast_node((struct ast_node*)node->combi_expr.lhs);
            free_ast_node((struct ast_node*)node->combi_expr.rhs);
            break;
        default: {
            switch_default_error("Invalid expr type");
        }
    }
    free(node);
}

bool get_variable(betree_var_t variable_id, const struct event* event, struct value* value)
{
    for(size_t i=0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(variable_id == pred->variable_id) {
            *value = pred->value;
            return true;
        }
    }
    return false;
}

double get_float_variable(betree_var_t variable_id, const struct event* event)
{
    struct value value;
    bool found = get_variable(variable_id, event, &value);
    if(!found) {
        fprintf(stderr, "Variable %llu not defined in event", variable_id);
        abort();
    }
    if(value.value_type != VALUE_F) {
        fprintf(stderr, "Variable %llu is not a float", variable_id);
        abort();
    }
    return value.fvalue;
}

double get_float_variable_str(struct config* config, const struct event* event, const char* name)
{
    betree_var_t variable_id = get_id_for_attr(config, name);
    return get_float_variable(variable_id, event);
}

struct string_value get_string_variable(betree_var_t variable_id, const struct event* event)
{
    struct value value;
    bool found = get_variable(variable_id, event, &value);
    if(!found) {
        fprintf(stderr, "Variable %llu not defined in event", variable_id);
        abort();
    }
    if(value.value_type != VALUE_S) {
        fprintf(stderr, "Variable %llu is not a string", variable_id);
        abort();
    }
    return value.svalue;
}

struct string_value get_string_variable_str(struct config* config, const struct event* event, const char* name)
{
    betree_var_t variable_id = get_id_for_attr(config, name);
    return get_string_variable(variable_id, event);
}

int64_t get_integer_variable(betree_var_t variable_id, const struct event* event)
{
    struct value value;
    bool found = get_variable(variable_id, event, &value);
    if(!found) {
        fprintf(stderr, "Variable %llu not defined in event", variable_id);
        abort();
    }
    if(value.value_type != VALUE_I) {
        fprintf(stderr, "Variable %llu is not an integer", variable_id);
        abort();
    }
    return value.ivalue;
}

int64_t get_integer_variable_str(struct config* config, const struct event* event, const char* name)
{
    betree_var_t variable_id = get_id_for_attr(config, name);
    return get_integer_variable(variable_id, event);
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

bool numeric_compare_value_matches(enum ast_numeric_compare_value_e a, enum value_e b) {
    return
        (a == AST_NUMERIC_COMPARE_VALUE_INTEGER && b == VALUE_I) ||
        (a == AST_NUMERIC_COMPARE_VALUE_FLOAT && b == VALUE_F);
}

bool equality_value_matches(enum ast_equality_value_e a, enum value_e b) {
    return
        (a == AST_EQUALITY_VALUE_INTEGER && b == VALUE_I) ||
        (a == AST_EQUALITY_VALUE_FLOAT && b == VALUE_F) ||
        (a == AST_EQUALITY_VALUE_STRING && b == VALUE_S);
}

bool list_value_matches(enum ast_list_value_e a, enum value_e b) {
    return
        (a == AST_LIST_VALUE_INTEGER_LIST && b == VALUE_IL) ||
        (a == AST_LIST_VALUE_STRING_LIST && b == VALUE_SL);
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

struct string_value frequency_type_to_string(struct config* config, enum ast_special_frequency_type_e type)
{
    const char* string;
    switch(type) {
        case AST_SPECIAL_TYPE_ADVERTISER: {
            string = "advertiser";
            break;
        }
        case AST_SPECIAL_TYPE_ADVERTISERIP: {
            string = "advertiser:ip";
            break;
        }
        case AST_SPECIAL_TYPE_CAMPAIGN: {
            string = "campaign";
            break;
        }
        case AST_SPECIAL_TYPE_CAMPAIGNIP: {
            string = "campaign:ip";
            break;
        }
        case AST_SPECIAL_TYPE_FLIGHT: {
            string = "flight";
            break;
        }
        case AST_SPECIAL_TYPE_FLIGHTIP: {
            string = "flight:ip";
            break;
        }
        case AST_SPECIAL_TYPE_PRODUCT: {
            string = "product";
            break;
        }
        case AST_SPECIAL_TYPE_PRODUCTIP: {
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

bool match_special_expr(struct config* config, const struct event* event, const struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY: {
            switch(special_expr.frequency.op) {
                case AST_SPECIAL_WITHINFREQUENCYCAP: {
                    int64_t now = get_integer_variable_str(config, event, "now");
                    const struct frequency_caps_list* caps = NULL;
                    uint32_t id;
                    struct string_value type = frequency_type_to_string(config, special_expr.frequency.type);
                    return within_frequency_caps(caps, type, id, special_expr.frequency.ns, special_expr.frequency.value, special_expr.frequency.length, now);
                }
                default: {
                    switch_default_error("Invalid frequency operation");
                    return false;
                }
            }
        }
        case AST_SPECIAL_SEGMENT: {
            int64_t now = get_integer_variable_str(config, event, "now");
            const struct segments_list* segments = NULL;
            switch(special_expr.segment.op) {
                case AST_SPECIAL_SEGMENTWITHIN: {
                    return segment_within(special_expr.segment.segment_id, special_expr.segment.seconds, segments, now);
                }
                case AST_SPECIAL_SEGMENTBEFORE: {
                    return segment_before(special_expr.segment.segment_id, special_expr.segment.seconds, segments, now);
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
                    double latitude_cst = get_geo_value_as_float(special_expr.geo.latitude);
                    double longitude_cst = get_geo_value_as_float(special_expr.geo.longitude);
                    double latitude_var = get_float_variable_str(config, event, "latitude");
                    double longitude_var = get_float_variable_str(config, event, "longitude");
                    double radius_cst = get_geo_value_as_float(special_expr.geo.radius);
                    return geo_within_radius(latitude_cst, longitude_cst, latitude_var, longitude_var, radius_cst);
                }
                default: {
                    switch_default_error("Invalid geo operation");
                    return false;
                }
            }
            return false;
        }
        case AST_SPECIAL_STRING: {
            struct string_value value = get_string_variable_str(config, event, special_expr.string.name);
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

bool match_bool_expr(const struct event* event, const struct ast_bool_expr bool_expr)
{
    struct value variable;
    bool found = get_variable(bool_expr.variable_id, event, &variable);
    if(!found) {
        return false;
    }
    if(variable.value_type != VALUE_B) {
        invalid_expr("boolean expression with a variable that is not a boolean");
    }
    switch(bool_expr.op) {
        case AST_BOOL_NONE: {
            return variable.bvalue;
        }
        case AST_BOOL_NOT: {
            return !variable.bvalue;
        }
        default: {
            switch_default_error("Invalid bool operation");
            return false;
        }
    }
}

bool match_list_expr(const struct event* event, const struct ast_list_expr list_expr)
{
    struct value variable;
    bool found = get_variable(list_expr.variable_id, event, &variable);
    if(!found) {
        return false;
    }
    if(!list_value_matches(list_expr.value.value_type, variable.value_type)) {
        invalid_expr("list value types do not match");
    }
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

bool match_set_expr(const struct event* event, const struct ast_set_expr set_expr)
{
    struct set_left_value left = set_expr.left_value;
    struct set_right_value right = set_expr.right_value;
    struct value variable;
    bool is_in;
    if(left.value_type == AST_SET_LEFT_VALUE_INTEGER && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        bool found = get_variable(right.variable_value.variable_id, event, &variable);
        if(!found) {
            return false;
        }
        if(variable.value_type != VALUE_IL) {
            invalid_expr("invalid variable in set expression, should be integer list");
        }
        is_in = integer_in_integer_list(left.integer_value, variable.ilvalue);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_STRING && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        bool found = get_variable(right.variable_value.variable_id, event, &variable);
        if(!found) {
            return false;
        }
        if(variable.value_type != VALUE_SL) {
            invalid_expr("invalid variable in set expression, should be string list");
        }
        is_in = string_in_string_list(left.string_value, variable.slvalue);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
        bool found = get_variable(left.variable_value.variable_id, event, &variable);
        if(!found) {
            return false;
        }
        if(variable.value_type != VALUE_I) {
            invalid_expr("invalid variable in set expression, should be integer");
        }
        is_in = integer_in_integer_list(variable.ivalue, right.integer_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
        bool found = get_variable(left.variable_value.variable_id, event, &variable);
        if(!found) {
            return false;
        }
        if(variable.value_type != VALUE_S) {
            invalid_expr("invalid variable in set expression, should be string");
        }
        is_in = string_in_string_list(variable.svalue, right.string_list_value);
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

bool match_numeric_compare_expr(const struct event* event, const struct ast_numeric_compare_expr numeric_compare_expr)
{
    struct value variable;
    bool found = get_variable(numeric_compare_expr.variable_id, event, &variable);
    if(!found) {
        return false;
    }
    if(!numeric_compare_value_matches(numeric_compare_expr.value.value_type, variable.value_type)) {
        invalid_expr("numeric compare value types do not match");
        return false;
    }
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

bool match_equality_expr(const struct event* event, const struct ast_equality_expr equality_expr)
{
    struct value variable;
    bool found = get_variable(equality_expr.variable_id, event, &variable);
    if(!found) {
        return false;
    }
    if(!equality_value_matches(equality_expr.value.value_type, variable.value_type)) {
        invalid_expr("equality value types do not match");
        return false;
    }
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

bool match_combi_expr(struct config* config, const struct event* event, const struct ast_combi_expr combi_expr)
{
    bool lhs = match_node(config, event, combi_expr.lhs);
    switch(combi_expr.op) {
        case AST_COMBI_AND: {
            if(lhs == false) {
                return false;
            }
            bool rhs = match_node(config, event, combi_expr.rhs);
            return lhs && rhs;
        }
        case AST_COMBI_OR: {
            bool rhs = match_node(config, event, combi_expr.rhs);
            return lhs || rhs;
        }
        default: {
            switch_default_error("Invalid combi operation");
            return false;
        }
    }
}

bool match_node(struct config* config, const struct event* event, const struct ast_node *node)
{
    // TODO allow undefined handling?
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            return match_special_expr(config, event, node->special_expr);
        }
        case AST_TYPE_BOOL_EXPR: {
            return match_bool_expr(event, node->bool_expr);
        }
        case AST_TYPE_LIST_EXPR: {
            return match_list_expr(event, node->list_expr);
        }
        case AST_TYPE_SET_EXPR: {
            return match_set_expr(event, node->set_expr);
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            return match_numeric_compare_expr(event, node->numeric_compare_expr);
        }
        case AST_TYPE_EQUALITY_EXPR: {
            return match_equality_expr(event, node->equality_expr);
        }
        case AST_TYPE_COMBI_EXPR: {
            return match_combi_expr(config, event, node->combi_expr);
        }
        default: {
            switch_default_error("Invalid expr type");
            return false;
        }
    }
}

void get_variable_bound(const struct attr_domain* domain, const struct ast_node* node, struct value_bound* bound)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            invalid_expr("Trying t get a bound of an special expression");
            return;
        }
        case AST_TYPE_COMBI_EXPR: {
            get_variable_bound(domain, node->combi_expr.lhs, bound);
            get_variable_bound(domain, node->combi_expr.rhs, bound);
            return;
        }
        case AST_TYPE_LIST_EXPR: {
            invalid_expr("Trying to get the bound of an list expression");
            return;
        }
        case AST_TYPE_SET_EXPR: {
            invalid_expr("Trying to get the bound of an set expression");
            return;
        }
        case AST_TYPE_BOOL_EXPR: {
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    bound->bmin = min(bound->bmin, true);
                    bound->bmax = max(bound->bmax, true);
                    return;
                }
                case AST_BOOL_NOT: {
                    bound->bmin = min(bound->bmin, false);
                    bound->bmax = max(bound->bmax, false);
                    return;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return;
                }
            }
        }
        case AST_TYPE_EQUALITY_EXPR: {
            if(domain->bound.value_type != bound->value_type || 
              !equality_value_matches(node->equality_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
                return;
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            bound->imin = min(bound->imin, node->equality_expr.value.integer_value);
                            bound->imax = max(bound->imax, node->equality_expr.value.integer_value);
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
            if(domain->bound.value_type != bound->value_type || 
              !numeric_compare_value_matches(node->numeric_compare_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain, bound or expr type mismatch");
                return;
            }
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            bound->imin = domain->bound.imin;
                            bound->imax = max(bound->imax, node->numeric_compare_expr.value.integer_value - 1);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value - __DBL_EPSILON__);
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
                            bound->imax = max(bound->imax, node->numeric_compare_expr.value.integer_value);
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = domain->bound.fmin;
                            bound->fmax = fmax(bound->fmax, node->numeric_compare_expr.value.float_value);
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
                            bound->imin = min(bound->imin, node->numeric_compare_expr.value.integer_value + 1);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value + __DBL_EPSILON__);
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
                            bound->imin = min(bound->imin, node->numeric_compare_expr.value.integer_value);
                            bound->imax = domain->bound.imax;
                            return;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            bound->fmin = fmin(bound->fmin, node->numeric_compare_expr.value.float_value);
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
                    return;
                }
                case AST_SPECIAL_SEGMENT: {
                    if(node->special_expr.segment.has_variable) {
                        betree_var_t variable_id = get_id_for_attr(config, node->special_expr.segment.name);
                        node->special_expr.segment.variable_id = variable_id;
                    }
                    return;
                }
                case AST_SPECIAL_GEO: {
                    return;
                }
                case AST_SPECIAL_STRING: {
                    betree_var_t variable_id = get_id_for_attr(config, node->special_expr.string.name);
                    node->special_expr.string.variable_id = variable_id;
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
            betree_var_t variable_id = get_id_for_attr(config, node->numeric_compare_expr.name);
            node->numeric_compare_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->equality_expr.name);
            node->equality_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->bool_expr.name);
            node->bool_expr.variable_id = variable_id;
            return;
        }
        case(AST_TYPE_LIST_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->list_expr.name);
            node->list_expr.variable_id = variable_id;
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
                    betree_var_t variable_id = get_id_for_attr(config, node->set_expr.left_value.variable_value.name);
                    node->set_expr.left_value.variable_value.variable_id = variable_id;
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
                    betree_var_t variable_id = get_id_for_attr(config, node->set_expr.right_value.variable_value.name);
                    node->set_expr.right_value.variable_value.variable_id = variable_id;
                    break;
                }
                default: {
                    switch_default_error("Invalid set right value type");
                }
            }
            return;
        }
        case(AST_TYPE_COMBI_EXPR): {
            assign_variable_id(config, (struct ast_node*)node->combi_expr.lhs);
            assign_variable_id(config, (struct ast_node*)node->combi_expr.rhs);
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
                    betree_str_t str_id = get_id_for_string(config, node->special_expr.frequency.ns.string);
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
            }
            return;
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config, node->equality_expr.value.string_value.string);
                node->equality_expr.value.string_value.str = str_id;
            }
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            return;
        }
        case(AST_TYPE_LIST_EXPR): {
            if(node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->list_expr.value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(config, node->list_expr.value.string_list_value.strings[i].string);
                    node->list_expr.value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config, node->set_expr.left_value.string_value.string);
                node->set_expr.left_value.string_value.str = str_id;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->set_expr.right_value.string_list_value.count; i++) {
                    betree_str_t str_id = get_id_for_string(config, node->set_expr.right_value.string_list_value.strings[i].string);
                    node->set_expr.right_value.string_list_value.strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_COMBI_EXPR): {
            assign_str_id(config, (struct ast_node*)node->combi_expr.lhs);
            assign_str_id(config, (struct ast_node*)node->combi_expr.rhs);
            return;
        }
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

const char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            invalid_expr("TODO");
            return NULL;
        }
        case(AST_TYPE_COMBI_EXPR): {
            const char* a = ast_to_string(node->combi_expr.lhs);
            const char* b = ast_to_string(node->combi_expr.rhs);
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    asprintf(&expr, "%s && %s", a, b);
                    break;
                }
                case AST_COMBI_OR: {
                    asprintf(&expr, "%s || %s", a, b);
                    break;
                }
                default: {
                    switch_default_error("Invalid combi operation");
                }
            }
            free((char*)a);
            free((char*)b);
            return expr;
        }
        case(AST_TYPE_SET_EXPR): {
            switch(node->set_expr.left_value.value_type) {
                case AST_SET_LEFT_VALUE_INTEGER: {
                    asprintf(&expr, "%lld ", node->set_expr.left_value.integer_value);
                    break;
                }
                case AST_SET_LEFT_VALUE_STRING: {
                    asprintf(&expr, "\"%s\" ", node->set_expr.left_value.string_value.string);
                    break;
                }
                case AST_SET_LEFT_VALUE_VARIABLE: {
                    asprintf(&expr, "%s ", node->set_expr.left_value.variable_value.name);
                    break;
                }
                default: {
                    switch_default_error("Invalid set left value type");
                }
            }
            switch(node->set_expr.op) {
                case AST_SET_NOT_IN: {
                    asprintf(&expr, "not in ");
                    break;
                }
                case AST_SET_IN: {
                    asprintf(&expr, "in ");
                    break;
                }
                default: {
                    switch_default_error("Invalid set operation");
                }
            }
            switch(node->set_expr.right_value.value_type) {
                case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                    const char* list = integer_list_value_to_string(node->set_expr.right_value.integer_list_value);
                    asprintf(&expr, "(%s) ", list);
                    free((char*)list);
                    break;
                }
                case AST_SET_RIGHT_VALUE_STRING_LIST: {
                    const char* list = string_list_value_to_string(node->set_expr.right_value.string_list_value);
                    asprintf(&expr, "(%s) ", list);
                    free((char*)list);
                    break;
                }
                case AST_SET_RIGHT_VALUE_VARIABLE: {
                    asprintf(&expr, "%s ", node->set_expr.right_value.variable_value.name);
                    break;
                }
                default: {
                    switch_default_error("Invalid set right value type");
                }
            }
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            char* list;
            switch(node->list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST: {
                    const char* inner = integer_list_value_to_string(node->list_expr.value.integer_list_value);
                    asprintf(&list, "(%s) ", inner);
                    free((char*)inner);
                    break;
                }
                case AST_LIST_VALUE_STRING_LIST: {
                    const char* inner = string_list_value_to_string(node->list_expr.value.string_list_value);
                    asprintf(&list, "(%s) ", inner);
                    free((char*)inner);
                    break;
                }
                default: {
                    switch_default_error("Invalid list value type");
                }
            }
            char* op;
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF: {
                    asprintf(&op, "one of ");
                    break;
                }
                case AST_LIST_NONE_OF: {
                    asprintf(&op, "none of ");
                    break;
                }
                case AST_LIST_ALL_OF: {
                    asprintf(&op, "all of ");
                    break;
                }
                default: {
                    switch_default_error("Invalid list operation");
                }
            }
            asprintf(&expr, "%s %s %s", node->list_expr.name, op, list);
            free(list);
            free(op);
            return expr;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    asprintf(&expr, "%s", node->bool_expr.name);
                    return expr;
                }
                case AST_BOOL_NOT: {
                    asprintf(&expr, "not %s", node->bool_expr.name);
                    return expr;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return NULL;
                }
            }
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            asprintf(&expr, "%s = %lld", node->equality_expr.name, node->equality_expr.value.integer_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            asprintf(&expr, "%s = %.2f", node->equality_expr.name, node->equality_expr.value.float_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            asprintf(&expr, "%s = \"%s\"", node->equality_expr.name, node->equality_expr.value.string_value.string);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid equality value type");
                            return NULL;
                        }
                    }
                }
                case AST_EQUALITY_NE: {
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER: {
                            asprintf(&expr, "%s <> %lld", node->equality_expr.name, node->equality_expr.value.integer_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_FLOAT: {
                            asprintf(&expr, "%s <> %.2f", node->equality_expr.name, node->equality_expr.value.float_value);
                            return expr;
                        }
                        case AST_EQUALITY_VALUE_STRING: {
                            asprintf(&expr, "%s <> \"%s\"", node->equality_expr.name, node->equality_expr.value.string_value.string);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid equality value type");
                            return NULL;
                        }
                    }
                }
                default: {
                    switch_default_error("Invalid equality operation");
                    return NULL;
                }
            }
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            switch(node->numeric_compare_expr.op) {
                case AST_NUMERIC_COMPARE_LT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s < %lld", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s < %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return NULL;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_LE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s <= %lld", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s <= %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return NULL;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GT: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s > %lld", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s > %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return NULL;
                        }
                    }
                }
                case AST_NUMERIC_COMPARE_GE: {
                    switch(node->numeric_compare_expr.value.value_type) {
                        case AST_NUMERIC_COMPARE_VALUE_INTEGER: {
                            asprintf(&expr, "%s >= %lld", node->numeric_compare_expr.name, node->numeric_compare_expr.value.integer_value);
                            return expr;
                        }
                        case AST_NUMERIC_COMPARE_VALUE_FLOAT: {
                            asprintf(&expr, "%s >= %.2f", node->numeric_compare_expr.name, node->numeric_compare_expr.value.float_value);
                            return expr;
                        }
                        default: {
                            switch_default_error("Invalid numeric compare value type");
                            return NULL;
                        }
                    }
                }
                default: {
                    switch_default_error("Invalid numeric compare operation");
                    return NULL;
                }
            }
        }
        default: {
            switch_default_error("Invalid expr type");
            return NULL;
        }
    }
}

