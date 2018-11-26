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
    node->id = INVALID_PRED;
    node->use_pred = false;
    return node;
}

struct ast_node* ast_is_null_expr_create(enum ast_is_null_e type, const char* name)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_IS_NULL_EXPR;
    node->is_null_expr.type = type;
    node->is_null_expr.attr_var = make_attr_var(name, NULL);
    return node;
}

struct ast_node* ast_compare_expr_create(
    enum ast_compare_e op, const char* name, struct compare_value value)
{
    struct ast_node* node = ast_node_create();
    node->type = AST_TYPE_COMPARE_EXPR;
    node->compare_expr.op = op;
    node->compare_expr.attr_var = make_attr_var(name, NULL);
    node->compare_expr.value = value;
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

struct ast_node* ast_bool_expr_literal_create(bool value)
{
    struct ast_node* node = ast_bool_expr_create(AST_BOOL_LITERAL);
    node->bool_expr.literal = value;
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

// Instant here means simply fetching a variable
enum scores {
    instant_cost = 1,
    single_length_cost = 5,
    double_length_cost = 10,
    complex_cost = 20,
};

static uint64_t score_node(struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            // faster than instant
            return instant_cost - 1;
        case AST_TYPE_COMPARE_EXPR:
            return instant_cost;
        case AST_TYPE_EQUALITY_EXPR:
            return instant_cost;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    // lhs + rhs
                    return score_node(node->bool_expr.binary.lhs)
                         + score_node(node->bool_expr.binary.rhs);
                case AST_BOOL_NOT:
                    // expr
                    return score_node(node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    // instant
                    return instant_cost;
                case AST_BOOL_LITERAL:
                    // faster than instant
                    return instant_cost - 1;
                default:
                    abort();
            }
        case AST_TYPE_SET_EXPR:
            // depend on constant length
            return single_length_cost;
        case AST_TYPE_LIST_EXPR:
            // depend on constant and variable length
            switch(node->list_expr.op) {
                case AST_LIST_ONE_OF:
                    return double_length_cost;
                case AST_LIST_NONE_OF:
                    return double_length_cost;
                case AST_LIST_ALL_OF:
                    return double_length_cost;
                default:
                    abort();
            }
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    // depend on variable length
                    return complex_cost;
                case AST_SPECIAL_SEGMENT:
                    // depend on variable length
                    return complex_cost;
                case AST_SPECIAL_GEO:
                    // instant
                    return instant_cost;
                case AST_SPECIAL_STRING:
                    // depend on constant and variable length
                    return complex_cost;
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
        .length = length,
        .id = UINT32_MAX };
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
    double latitude,
    double longitude,
    bool has_radius,
    double radius)
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

static void free_special_expr(struct ast_special_expr special_expr)
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

static void free_set_expr(struct ast_set_expr set_expr)
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
            free_integer_list(set_expr.right_value.integer_list_value);
            break;
        }
        case AST_SET_RIGHT_VALUE_STRING_LIST: {
            free_string_list(set_expr.right_value.string_list_value);
            break;
        }
        case AST_SET_RIGHT_VALUE_VARIABLE: {
            free_attr_var(set_expr.right_value.variable_value);
            break;
        }
        case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
            free_integer_list_enum(set_expr.right_value.integer_list_enum_value);
            break;
        default: {
            switch_default_error("Invalid set right value type");
        }
    }
}

static void free_list_expr(struct ast_list_expr list_expr)
{
    switch(list_expr.value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            free_integer_list(list_expr.value.integer_list_value);
            break;
        }
        case AST_LIST_VALUE_STRING_LIST: {
            free_string_list(list_expr.value.string_list_value);
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
        case AST_TYPE_IS_NULL_EXPR:
            free_attr_var(node->is_null_expr.attr_var);
            break;
        case AST_TYPE_SPECIAL_EXPR:
            free_special_expr(node->special_expr);
            break;
        case AST_TYPE_COMPARE_EXPR:
            free_attr_var(node->compare_expr.attr_var);
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
                case AST_BOOL_LITERAL:
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

bool d64binary_search(int64_t arr[], size_t count, int64_t to_find)
{
    int imin = 0;
    int imax = (int)count - 1;
    while(imax >= imin) {
        int imid = imin + ((imax - imin) / 2);
        if(arr[imid] == to_find) {
            return true;
        }
        else if(arr[imid] < to_find) {
            imin = imid + 1;
        }
        else {
            imax = imid - 1;
        }
    }
    return false;
}

bool sbinary_search(struct string_value arr[], size_t count, betree_str_t to_find)
{
    int imin = 0;
    int imax = (int)count - 1;
    while(imax >= imin) {
        int imid = imin + ((imax - imin) / 2);
        if(arr[imid].str == to_find) {
            return true;
        }
        else if(arr[imid].str < to_find) {
            imin = imid + 1;
        }
        else {
            imax = imid - 1;
        }
    }
    return false;
}

bool iebinary_search(struct integer_enum_value arr[], size_t count, betree_ienum_t to_find)
{
    int imin = 0;
    int imax = (int)count - 1;
    while(imax >= imin) {
        int imid = imin + ((imax - imin) / 2);
        if(arr[imid].ienum == to_find) {
            return true;
        }
        else if(arr[imid].ienum < to_find) {
            imin = imid + 1;
        }
        else {
            imax = imid - 1;
        }
    }
    return false;
}

static bool integer_in_integer_list(int64_t integer, struct betree_integer_list* list)
{
    return d64binary_search(list->integers, list->count, integer);
}

static bool string_in_string_list(struct string_value string, struct betree_string_list* list)
{
    return sbinary_search(list->strings, list->count, string.str);
}

static bool integer_enum_in_integer_list_enum(struct integer_enum_value ienum, struct betree_integer_list_enum* list)
{
    return iebinary_search(list->integers, list->count, ienum.ienum);
}

static bool compare_value_matches(enum ast_compare_value_e a, enum betree_value_type_e b)
{
    return (a == AST_COMPARE_VALUE_INTEGER && b == BETREE_INTEGER)
        || (a == AST_COMPARE_VALUE_FLOAT && b == BETREE_FLOAT);
}

static bool equality_value_matches(enum ast_equality_value_e a, enum betree_value_type_e b)
{
    return (a == AST_EQUALITY_VALUE_INTEGER && b == BETREE_INTEGER)
        || (a == AST_EQUALITY_VALUE_FLOAT && b == BETREE_FLOAT)
        || (a == AST_EQUALITY_VALUE_STRING && b == BETREE_STRING)
        || (a == AST_EQUALITY_VALUE_INTEGER_ENUM && b == BETREE_INTEGER_ENUM);
}

static bool list_value_matches(enum ast_list_value_e a, enum betree_value_type_e b)
{
    return (a == AST_LIST_VALUE_INTEGER_LIST && b == BETREE_INTEGER_LIST)
        || (a == AST_LIST_VALUE_STRING_LIST && b == BETREE_STRING_LIST);
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

static bool match_special_expr(
    const struct betree_variable** preds, const struct ast_special_expr special_expr)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY: {
            switch(special_expr.frequency.op) {
                case AST_SPECIAL_WITHINFREQUENCYCAP: {
                    const struct ast_special_frequency* f = &special_expr.frequency;
                    struct betree_frequency_caps* caps;
                    bool is_caps_defined = get_frequency_var(f->attr_var.var, preds, &caps);
                    if(is_caps_defined == false) {
                        return false;
                    }
                    if(caps->size == 0) {
                        // Optimization from looking at what within_frequency_caps does
                        return true;
                    }
                    int64_t now;
                    bool is_now_defined = get_integer_var(f->now.var, preds, &now);
                    if(is_now_defined == false) {
                        return false;
                    }
                    return within_frequency_caps(
                        caps, f->type, f->id, f->ns, f->value, f->length, now);
                }
                default:
                    switch_default_error("Invalid frequency operation");
                    return false;
            }
        }
        case AST_SPECIAL_SEGMENT: {
            const struct ast_special_segment* s = &special_expr.segment;
            struct betree_segments* segments;
            bool is_segment_defined = get_segments_var(s->attr_var.var, preds, &segments);
            if(is_segment_defined == false) {
                return false;
            }
            int64_t now;
            bool is_now_defined = get_integer_var(s->now.var, preds, &now);
            if(is_now_defined == false) {
                return false;
            }
            switch(special_expr.segment.op) {
                case AST_SPECIAL_SEGMENTWITHIN:
                    return segment_within(s->segment_id, s->seconds, segments, now);
                case AST_SPECIAL_SEGMENTBEFORE:
                    return segment_before(s->segment_id, s->seconds, segments, now);
                default:
                    switch_default_error("Invalid segment operation");
                    return false;
            }
        }
        case AST_SPECIAL_GEO: {
            switch(special_expr.geo.op) {
                case AST_SPECIAL_GEOWITHINRADIUS: {
                    const struct ast_special_geo* g = &special_expr.geo;
                    double latitude_var, longitude_var;
                    bool is_latitude_defined
                        = get_float_var(g->latitude_var.var, preds, &latitude_var);
                    bool is_longitude_defined
                        = get_float_var(g->longitude_var.var, preds, &longitude_var);
                    if(is_latitude_defined == false || is_longitude_defined == false) {
                        return false;
                    }

                    return geo_within_radius(
                        g->latitude, g->longitude, latitude_var, longitude_var, g->radius);
                }
                default:
                    switch_default_error("Invalid geo operation");
                    return false;
            }
            return false;
        }
        case AST_SPECIAL_STRING: {
            const struct ast_special_string* s = &special_expr.string;
            struct string_value value;
            bool is_string_defined = get_string_var(s->attr_var.var, preds, &value);
            if(is_string_defined == false) {
                return false;
            }
            switch(s->op) {
                case AST_SPECIAL_CONTAINS:
                    return contains(value.string, s->pattern);
                case AST_SPECIAL_STARTSWITH:
                    return starts_with(value.string, s->pattern);
                case AST_SPECIAL_ENDSWITH:
                    return ends_with(value.string, s->pattern);
                default:
                    switch_default_error("Invalid string operation");
                    return false;
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
    if(variable.ilvalue->count < list_expr.value.integer_list_value->count) {
        xs = variable.ilvalue->integers;
        x_count = variable.ilvalue->count;
        ys = list_expr.value.integer_list_value->integers;
        y_count = list_expr.value.integer_list_value->count;
    }
    else {
        ys = variable.ilvalue->integers;
        y_count = variable.ilvalue->count;
        xs = list_expr.value.integer_list_value->integers;
        x_count = list_expr.value.integer_list_value->count;
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
    if(variable.slvalue->count < list_expr.value.integer_list_value->count) {
        xs = variable.slvalue->strings;
        x_count = variable.slvalue->count;
        ys = list_expr.value.string_list_value->strings;
        y_count = list_expr.value.integer_list_value->count;
    }
    else {
        ys = variable.slvalue->strings;
        y_count = variable.slvalue->count;
        xs = list_expr.value.string_list_value->strings;
        x_count = list_expr.value.integer_list_value->count;
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
    int64_t* xs = list_expr.value.integer_list_value->integers;
    size_t x_count = list_expr.value.integer_list_value->count;
    int64_t* ys = variable.ilvalue->integers;
    size_t y_count = variable.ilvalue->count;
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
    struct string_value* xs = list_expr.value.string_list_value->strings;
    size_t x_count = list_expr.value.string_list_value->count;
    struct string_value* ys = variable.slvalue->strings;
    size_t y_count = variable.slvalue->count;
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

static bool match_list_expr(
    const struct betree_variable** preds, const struct ast_list_expr list_expr)
{
    struct value variable;
    bool is_variable_defined = get_variable(list_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
        return false;
    }
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

static bool match_set_expr(const struct betree_variable** preds, const struct ast_set_expr set_expr)
{
    struct set_left_value left = set_expr.left_value;
    struct set_right_value right = set_expr.right_value;
    bool is_in;
    if(left.value_type == AST_SET_LEFT_VALUE_INTEGER
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct betree_integer_list* variable;
        bool is_variable_defined = get_integer_list_var(right.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = integer_in_integer_list(left.integer_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_STRING
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        struct betree_string_list* variable;
        bool is_variable_defined = get_string_list_var(right.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = string_in_string_list(left.string_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
        int64_t variable;
        bool is_variable_defined = get_integer_var(left.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = integer_in_integer_list(variable, right.integer_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
        struct string_value variable;
        bool is_variable_defined = get_string_var(left.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = string_in_string_list(variable, right.string_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
      && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM) {
        struct integer_enum_value variable;
        bool is_variable_defined = get_integer_enum_var(left.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = integer_enum_in_integer_list_enum(variable, right.integer_list_enum_value);
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

static bool match_compare_expr(
    const struct betree_variable** preds, const struct ast_compare_expr compare_expr)
{
    struct value variable;
    bool is_variable_defined = get_variable(compare_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
        return false;
    }
    switch(compare_expr.op) {
        case AST_COMPARE_LT: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue < compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue < compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid compare value type");
                    return false;
                }
            }
        }
        case AST_COMPARE_LE: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue <= compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue <= compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid compare value type");
                    return false;
                }
            }
        }
        case AST_COMPARE_GT: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue > compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue > compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid compare value type");
                    return false;
                }
            }
        }
        case AST_COMPARE_GE: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.ivalue >= compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.fvalue >= compare_expr.value.float_value;
                    return result;
                }
                default: {
                    switch_default_error("Invalid compare value type");
                    return false;
                }
            }
        }
        default: {
            switch_default_error("Invalid compare operation");
            return false;
        }
    }
}

static bool match_equality_expr(
    const struct betree_variable** preds, const struct ast_equality_expr equality_expr)
{
    struct value variable;
    bool is_variable_defined = get_variable(equality_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
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
                case AST_EQUALITY_VALUE_INTEGER_ENUM: {
                    bool result = variable.ievalue.ienum == equality_expr.value.integer_enum_value.ienum;
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
                case AST_EQUALITY_VALUE_INTEGER_ENUM: {
                    bool result = variable.ievalue.ienum != equality_expr.value.integer_enum_value.ienum;
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

static bool match_node_inner(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report* report);

static bool match_bool_expr(const struct betree_variable** preds,
    const struct ast_bool_expr bool_expr,
    struct memoize* memoize,
    struct report* report)
{
    switch(bool_expr.op) {
        case AST_BOOL_LITERAL:
            return bool_expr.literal;
        case AST_BOOL_AND: {
            bool lhs = match_node_inner(preds, bool_expr.binary.lhs, memoize, report);
            if(lhs == false) {
                return false;
            }
            bool rhs = match_node_inner(preds, bool_expr.binary.rhs, memoize, report);
            return rhs;
        }
        case AST_BOOL_OR: {
            bool lhs = match_node_inner(preds, bool_expr.binary.lhs, memoize, report);
            if(lhs == true) {
                return true;
            }
            bool rhs = match_node_inner(preds, bool_expr.binary.rhs, memoize, report);
            return rhs;
        }
        case AST_BOOL_NOT: {
            bool result = match_node_inner(preds, bool_expr.unary.expr, memoize, report);
            return !result;
        }
        case AST_BOOL_VARIABLE: {
            bool value;
            bool is_variable_defined = get_bool_var(bool_expr.variable.var, preds, &value);
            if(is_variable_defined == false) {
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

static bool match_is_null_expr(const struct betree_variable** preds,
    const struct ast_is_null_expr is_null_expr)
{
    struct value variable;
    bool is_variable_defined = get_variable(is_null_expr.attr_var.var, preds, &variable);
    switch(is_null_expr.type) {
        case AST_IS_NULL:
            return !is_variable_defined;
        case AST_IS_NOT_NULL:
            return is_variable_defined;
        case AST_IS_EMPTY:
            return is_variable_defined && is_empty_list(variable);
        default:
            switch_default_error("Invalid is null operation");
            return false;
    }
}

static bool match_node_inner(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report* report)
{
    if(node->use_pred && node->id != INVALID_PRED) {
        if(test_bit(memoize->pass, node->id)) {
            report->memoized++;
            return true;
        }
        if(test_bit(memoize->fail, node->id)) {
            report->memoized++;
            return false;
        }
    }
    bool result;
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            result = match_is_null_expr(preds, node->is_null_expr);
            break;
        case AST_TYPE_SPECIAL_EXPR: {
            result = match_special_expr(preds, node->special_expr);
            break;
        }
        case AST_TYPE_BOOL_EXPR: {
            result = match_bool_expr(preds, node->bool_expr, memoize, report);
            break;
        }
        case AST_TYPE_LIST_EXPR: {
            result = match_list_expr(preds, node->list_expr);
            break;
        }
        case AST_TYPE_SET_EXPR: {
            result = match_set_expr(preds, node->set_expr);
            break;
        }
        case AST_TYPE_COMPARE_EXPR: {
            result = match_compare_expr(preds, node->compare_expr);
            break;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            result = match_equality_expr(preds, node->equality_expr);
            break;
        }
        default: {
            switch_default_error("Invalid expr type");
            return false;
        }
    }
    if(node->use_pred && node->id != INVALID_PRED) {
        if(result) {
            set_bit(memoize->pass, node->id);
        }
        else {
            set_bit(memoize->fail, node->id);
        }
    }
    return result;
}

bool match_node(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report* report)
{
    return match_node_inner(preds, node, memoize, report);
}

struct bound_dirty {
    bool min_dirty;
    bool max_dirty;
};

static void get_variable_bound_inner(const struct attr_domain* domain,
    const struct ast_node* node,
    struct value_bound* bound,
    bool is_reversed,
    struct bound_dirty* dirty)
{
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            return;
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
                    if(domain->bound.value_type == BETREE_INTEGER_LIST
                        && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST) {
                        if(is_reversed) {
                        }
                        else {
                            if(node->list_expr.value.integer_list_value->count != 0) {
                                bound->imin = node->list_expr.value.integer_list_value->integers[0];
                                dirty->min_dirty = true;
                                bound->imax
                                    = node->list_expr.value.integer_list_value
                                          ->integers[node->list_expr.value.integer_list_value->count
                                              - 1];
                                dirty->max_dirty = true;
                            }
                            else {
                                return;
                            }
                        }
                        return;
                    }
                    else if(domain->bound.value_type == BETREE_STRING_LIST
                        && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                        if(is_reversed) {
                        }
                        else {
                            if(node->list_expr.value.string_list_value->count != 0) {
                                bound->smin
                                    = node->list_expr.value.string_list_value->strings[0].str;
                                dirty->min_dirty = true;
                                bound->smax
                                    = node->list_expr.value.string_list_value
                                          ->strings[node->list_expr.value.string_list_value->count
                                              - 1]
                                          .str;
                                dirty->max_dirty = true;
                            }
                            else {
                                return;
                            }
                        }
                        return;
                    }
                    else {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                case AST_LIST_NONE_OF:
                    if(domain->bound.value_type == BETREE_INTEGER_LIST
                        && node->list_expr.value.value_type == AST_LIST_VALUE_INTEGER_LIST) {
                        if(is_reversed) {
                            if(node->list_expr.value.integer_list_value->count != 0) {
                                bound->imin = node->list_expr.value.integer_list_value->integers[0];
                                dirty->min_dirty = true;
                                bound->imax
                                    = node->list_expr.value.integer_list_value
                                          ->integers[node->list_expr.value.integer_list_value->count
                                              - 1];
                                dirty->max_dirty = true;
                            }
                            else {
                                return;
                            }
                        }
                        else {
                        }
                        return;
                    }
                    else if(domain->bound.value_type == BETREE_STRING_LIST
                        && node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                        if(is_reversed) {
                            if(node->list_expr.value.string_list_value->count != 0) {
                                bound->smin
                                    = node->list_expr.value.string_list_value->strings[0].str;
                                dirty->min_dirty = true;
                                bound->smax
                                    = node->list_expr.value.string_list_value
                                          ->strings[node->list_expr.value.string_list_value->count
                                              - 1]
                                          .str;
                                dirty->max_dirty = true;
                            }
                            else {
                                return;
                            }
                        }
                        else {
                        }
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
                        if(domain->bound.value_type == BETREE_INTEGER
                            && node->set_expr.right_value.value_type
                                == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
                            if(is_reversed) {
                            }
                            else {
                                if(node->set_expr.right_value.integer_list_value->count != 0) {
                                    bound->imin = node->set_expr.right_value.integer_list_value
                                                      ->integers[0];
                                    dirty->min_dirty = true;
                                    bound->imax = node->set_expr.right_value.integer_list_value
                                                      ->integers[node->set_expr.right_value
                                                                     .integer_list_value->count
                                                          - 1];
                                    dirty->max_dirty = true;
                                }
                                else {
                                    return;
                                }
                            }
                            return;
                        }
                        else if(domain->bound.value_type == BETREE_STRING
                            && node->set_expr.right_value.value_type
                                == AST_SET_RIGHT_VALUE_STRING_LIST) {
                            if(is_reversed) {
                            }
                            else {
                                if(node->set_expr.right_value.string_list_value->count != 0) {
                                    bound->smin
                                        = node->set_expr.right_value.string_list_value->strings[0]
                                              .str;
                                    dirty->min_dirty = true;
                                    bound->smax = node->set_expr.right_value.string_list_value
                                                      ->strings[node->set_expr.right_value
                                                                    .string_list_value->count
                                                          - 1]
                                                      .str;
                                    dirty->max_dirty = true;
                                }
                                else {
                                    return;
                                }
                            }
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
                        if(domain->bound.value_type == BETREE_INTEGER_LIST
                            && node->set_expr.left_value.value_type
                                == AST_SET_LEFT_VALUE_INTEGER) {
                            if(is_reversed) {
                            }
                            else {
                                bound->imin = node->set_expr.left_value.integer_value;
                                dirty->min_dirty = true;
                                bound->imax = node->set_expr.left_value.integer_value;
                                dirty->max_dirty = true;
                            }
                            return;
                        }
                        else if(domain->bound.value_type == BETREE_STRING_LIST
                            && node->set_expr.left_value.value_type
                                == AST_SET_LEFT_VALUE_STRING) {
                            if(is_reversed) {
                            }
                            else {
                                bound->smin = node->set_expr.left_value.string_value.str;
                                dirty->min_dirty = true;
                                bound->smax = node->set_expr.left_value.string_value.str;
                                dirty->max_dirty = true;
                            }
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
                        if(domain->bound.value_type == BETREE_INTEGER
                            && node->set_expr.right_value.value_type
                                == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.integer_list_value->count != 0) {
                                    bound->imin = node->set_expr.right_value.integer_list_value
                                                      ->integers[0];
                                    dirty->min_dirty = true;
                                    bound->imax = node->set_expr.right_value.integer_list_value
                                                      ->integers[node->set_expr.right_value
                                                                     .integer_list_value->count
                                                          - 1];
                                    dirty->max_dirty = true;
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                            }
                            return;
                        }
                        else if(domain->bound.value_type == BETREE_STRING
                            && node->set_expr.right_value.value_type
                                == AST_SET_RIGHT_VALUE_STRING_LIST) {
                            if(is_reversed) {
                                if(node->set_expr.right_value.string_list_value->count != 0) {
                                    bound->smin
                                        = node->set_expr.right_value.string_list_value->strings[0]
                                              .str;
                                    dirty->min_dirty = true;
                                    bound->smax = node->set_expr.right_value.string_list_value
                                                      ->strings[node->set_expr.right_value
                                                                    .string_list_value->count
                                                          - 1]
                                                      .str;
                                    dirty->max_dirty = true;
                                }
                                else {
                                    return;
                                }
                            }
                            else {
                            }
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
                        if(domain->bound.value_type == BETREE_INTEGER_LIST
                            && node->set_expr.left_value.value_type
                                == AST_SET_LEFT_VALUE_INTEGER) {
                            if(is_reversed) {
                                bound->imin = node->set_expr.left_value.integer_value;
                                dirty->min_dirty = true;
                                bound->imax = node->set_expr.left_value.integer_value;
                                dirty->max_dirty = true;
                            }
                            else {
                            }
                            return;
                        }
                        else if(domain->bound.value_type == BETREE_STRING_LIST
                            && node->set_expr.left_value.value_type
                                == AST_SET_LEFT_VALUE_STRING) {
                            if(is_reversed) {
                                bound->smin = node->set_expr.left_value.string_value.str;
                                dirty->min_dirty = true;
                                bound->smax = node->set_expr.left_value.string_value.str;
                                dirty->max_dirty = true;
                            }
                            else {
                            }
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
                case AST_BOOL_LITERAL:
                    return;
                case AST_BOOL_VARIABLE:
                    if(domain->attr_var.var != node->bool_expr.variable.var) {
                        return;
                    }
                    if(domain->bound.value_type != BETREE_BOOLEAN) {
                        invalid_expr("Domain and expr type mismatch");
                        return;
                    }
                    if(is_reversed) {
                        bound->bmin = false;
                        dirty->min_dirty = true;
                        bound->bmax = false;
                        dirty->max_dirty = true;
                    }
                    else {
                        bound->bmin = true;
                        dirty->min_dirty = true;
                        bound->bmax = true;
                        dirty->max_dirty = true;
                    }
                    return;
                case AST_BOOL_NOT:
                    get_variable_bound_inner(
                        domain, node->bool_expr.unary.expr, bound, !is_reversed, dirty);
                    return;
                case AST_BOOL_OR: {
                    struct value_bound lbound = { .value_type = bound->value_type };
                    struct bound_dirty ldirty = { .min_dirty = false, .max_dirty = false };
                    struct value_bound rbound = { .value_type = bound->value_type };
                    struct bound_dirty rdirty = { .min_dirty = false, .max_dirty = false };
                    get_variable_bound_inner(
                        domain, node->bool_expr.binary.lhs, &lbound, is_reversed, &ldirty);
                    get_variable_bound_inner(
                        domain, node->bool_expr.binary.rhs, &rbound, is_reversed, &rdirty);
                    if(ldirty.min_dirty == true && rdirty.min_dirty == true) {
                        dirty->min_dirty = true;
                        switch(bound->value_type) {
                            case BETREE_BOOLEAN:
                                bound->bmin = lbound.bmin && rbound.bmin;
                                break;
                            case BETREE_INTEGER:
                            case BETREE_INTEGER_LIST:
                                bound->imin = d64min(lbound.imin, rbound.imin);
                                break;
                            case BETREE_FLOAT:
                                bound->fmin = fmin(lbound.fmin, rbound.fmin);
                                break;
                            case BETREE_STRING:
                            case BETREE_STRING_LIST:
                            case BETREE_INTEGER_ENUM:
                            case BETREE_INTEGER_LIST_ENUM:
                                bound->smin = smin(lbound.smin, rbound.smin);
                                break;
                            case BETREE_SEGMENTS:
                            case BETREE_FREQUENCY_CAPS:
                            default:
                                break;
                        }
                    }
                    if(ldirty.max_dirty == true && rdirty.max_dirty == true) {
                        dirty->max_dirty = true;
                        switch(bound->value_type) {
                            case BETREE_BOOLEAN:
                                bound->bmax = lbound.bmax || rbound.bmax;
                                break;
                            case BETREE_INTEGER:
                            case BETREE_INTEGER_LIST:
                                bound->imax = d64max(lbound.imax, rbound.imax);
                                break;
                            case BETREE_FLOAT:
                                bound->fmax = fmax(lbound.fmax, rbound.fmax);
                                break;
                            case BETREE_STRING:
                            case BETREE_STRING_LIST:
                            case BETREE_INTEGER_ENUM:
                            case BETREE_INTEGER_LIST_ENUM:
                                bound->smax = smax(lbound.smax, rbound.smax);
                                break;
                            case BETREE_SEGMENTS:
                            case BETREE_FREQUENCY_CAPS:
                            default:
                                break;
                        }
                    }
                    return;
                }
                case AST_BOOL_AND: {
                    struct value_bound lbound = { .value_type = bound->value_type };
                    struct bound_dirty ldirty = { .min_dirty = false, .max_dirty = false };
                    struct value_bound rbound = { .value_type = bound->value_type };
                    struct bound_dirty rdirty = { .min_dirty = false, .max_dirty = false };
                    get_variable_bound_inner(
                        domain, node->bool_expr.binary.lhs, &lbound, is_reversed, &ldirty);
                    get_variable_bound_inner(
                        domain, node->bool_expr.binary.rhs, &rbound, is_reversed, &rdirty);
                    if(ldirty.min_dirty == true || rdirty.min_dirty == true) {
                        dirty->min_dirty = true;
                        switch(bound->value_type) {
                            case BETREE_BOOLEAN:
                                if(ldirty.min_dirty == true && rdirty.min_dirty == true) {
                                    bound->bmin = lbound.bmin && rbound.bmin;
                                }
                                else if(ldirty.min_dirty == true) {
                                    bound->bmin = lbound.bmin;
                                }
                                else {
                                    bound->bmin = rbound.bmin;
                                }
                                break;
                            case BETREE_INTEGER:
                            case BETREE_INTEGER_LIST:
                                if(ldirty.min_dirty == true && rdirty.min_dirty == true) {
                                    bound->imin = d64min(lbound.imin, rbound.imin);
                                }
                                else if(ldirty.min_dirty == true) {
                                    bound->imin = lbound.imin;
                                }
                                else {
                                    bound->imin = rbound.imin;
                                }
                                break;
                            case BETREE_FLOAT:
                                if(ldirty.min_dirty == true && rdirty.min_dirty == true) {
                                    bound->fmin = fmin(lbound.fmin, rbound.fmin);
                                }
                                else if(ldirty.min_dirty == true) {
                                    bound->fmin = lbound.fmin;
                                }
                                else {
                                    bound->fmin = rbound.fmin;
                                }
                                break;
                            case BETREE_STRING:
                            case BETREE_STRING_LIST:
                            case BETREE_INTEGER_ENUM:
                            case BETREE_INTEGER_LIST_ENUM:
                                if(ldirty.min_dirty == true && rdirty.min_dirty == true) {
                                    bound->smin = smin(lbound.smin, rbound.smin);
                                }
                                else if(ldirty.min_dirty == true) {
                                    bound->smin = lbound.smin;
                                }
                                else {
                                    bound->smin = rbound.smin;
                                }
                                break;
                            case BETREE_SEGMENTS:
                            case BETREE_FREQUENCY_CAPS:
                            default:
                                break;
                        }
                    }
                    if(ldirty.max_dirty == true || rdirty.max_dirty == true) {
                        dirty->max_dirty = true;
                        switch(bound->value_type) {
                            case BETREE_BOOLEAN:
                                if(ldirty.max_dirty == true && rdirty.max_dirty == true) {
                                    bound->bmax = lbound.bmax || rbound.bmax;
                                }
                                else if(ldirty.max_dirty == true) {
                                    bound->bmax = lbound.bmax;
                                }
                                else {
                                    bound->bmax = rbound.bmax;
                                }
                                break;
                            case BETREE_INTEGER:
                            case BETREE_INTEGER_LIST:
                                if(ldirty.max_dirty == true && rdirty.max_dirty == true) {
                                    bound->imax = d64max(lbound.imax, rbound.imax);
                                }
                                else if(ldirty.max_dirty == true) {
                                    bound->imax = lbound.imax;
                                }
                                else {
                                    bound->imax = rbound.imax;
                                }
                                break;
                            case BETREE_FLOAT:
                                if(ldirty.max_dirty == true && rdirty.max_dirty == true) {
                                    bound->fmax = fmax(lbound.fmax, rbound.fmax);
                                }
                                else if(ldirty.max_dirty == true) {
                                    bound->fmax = lbound.fmax;
                                }
                                else {
                                    bound->fmax = rbound.fmax;
                                }
                                break;
                            case BETREE_STRING:
                            case BETREE_STRING_LIST:
                            case BETREE_INTEGER_ENUM:
                            case BETREE_INTEGER_LIST_ENUM:
                                if(ldirty.max_dirty == true && rdirty.max_dirty == true) {
                                    bound->smax = smax(lbound.smax, rbound.smax);
                                }
                                else if(ldirty.max_dirty == true) {
                                    bound->smax = lbound.smax;
                                }
                                else {
                                    bound->smax = rbound.smax;
                                }
                                break;
                            case BETREE_SEGMENTS:
                            case BETREE_FREQUENCY_CAPS:
                            default:
                                break;
                        }
                    }
                    return;
                }
                default:
                    switch_default_error("Invalid bool operation");
                    return;
            }
        case AST_TYPE_EQUALITY_EXPR:
            if(domain->attr_var.var != node->equality_expr.attr_var.var) {
                return;
            }
            if(!equality_value_matches(
                   node->equality_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain and expr type mismatch");
                return;
            }
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ:
                    switch(node->equality_expr.value.value_type) {
                        case AST_EQUALITY_VALUE_INTEGER:
                            if(is_reversed) {
                            }
                            else {
                                bound->imin = node->equality_expr.value.integer_value;
                                dirty->min_dirty = true;
                                bound->imax = node->equality_expr.value.integer_value;
                                dirty->max_dirty = true;
                            }
                            break;
                        case AST_EQUALITY_VALUE_FLOAT:
                            if(is_reversed) {
                            }
                            else {
                                bound->fmin = node->equality_expr.value.float_value;
                                dirty->min_dirty = true;
                                bound->fmax = node->equality_expr.value.float_value;
                                dirty->max_dirty = true;
                            }
                            break;
                        case AST_EQUALITY_VALUE_STRING:
                            if(is_reversed) {
                            }
                            else {
                                bound->smin = node->equality_expr.value.string_value.str;
                                dirty->min_dirty = true;
                                bound->smax = node->equality_expr.value.string_value.str;
                                dirty->max_dirty = true;
                            }
                            break;
                        case AST_EQUALITY_VALUE_INTEGER_ENUM:
                            if(is_reversed) {
                            }
                            else {
                                bound->smin = node->equality_expr.value.integer_enum_value.ienum;
                                dirty->min_dirty = true;
                                bound->smax = node->equality_expr.value.integer_enum_value.ienum;
                                dirty->max_dirty = true;
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
                                bound->imin = node->equality_expr.value.integer_value;
                                dirty->min_dirty = true;
                                bound->imax = node->equality_expr.value.integer_value;
                                dirty->max_dirty = true;
                            }
                            else {
                            }
                            break;
                        case AST_EQUALITY_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = node->equality_expr.value.float_value;
                                dirty->min_dirty = true;
                                bound->fmax = node->equality_expr.value.float_value;
                                dirty->max_dirty = true;
                            }
                            else {
                            }
                            break;
                        case AST_EQUALITY_VALUE_STRING:
                            if(is_reversed) {
                                bound->smin = node->equality_expr.value.string_value.str;
                                dirty->min_dirty = true;
                                bound->smax = node->equality_expr.value.string_value.str;
                                dirty->max_dirty = true;
                            }
                            else {
                            }
                            break;
                        case AST_EQUALITY_VALUE_INTEGER_ENUM:
                            if(is_reversed) {
                                bound->smin = node->equality_expr.value.integer_enum_value.ienum;
                                dirty->min_dirty = true;
                                bound->smax = node->equality_expr.value.integer_enum_value.ienum;
                                dirty->max_dirty = true;
                            }
                            else {
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
            return;
        case AST_TYPE_COMPARE_EXPR:
            if(domain->attr_var.var != node->compare_expr.attr_var.var) {
                return;
            }
            if(!compare_value_matches(
                   node->compare_expr.value.value_type, domain->bound.value_type)) {
                invalid_expr("Domain and expr type mismatch");
                return;
            }
            switch(node->compare_expr.op) {
                case AST_COMPARE_LT:
                    switch(node->compare_expr.value.value_type) {
                        case AST_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = node->compare_expr.value.integer_value;
                                dirty->min_dirty = true;
                            }
                            else {
                                bound->imax = node->compare_expr.value.integer_value - 1;
                                dirty->max_dirty = true;
                            }
                            break;
                        case AST_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin = node->compare_expr.value.float_value;
                                dirty->min_dirty = true;
                            }
                            else {
                                bound->fmax
                                    = node->compare_expr.value.float_value - __DBL_EPSILON__;
                                dirty->max_dirty = true;
                            }
                            break;
                        default:
                            switch_default_error("Invalid compare value type");
                            break;
                    }
                    break;
                case AST_COMPARE_LE:
                    switch(node->compare_expr.value.value_type) {
                        case AST_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imin = node->compare_expr.value.integer_value + 1;
                                dirty->min_dirty = true;
                            }
                            else {
                                bound->imax = node->compare_expr.value.integer_value;
                                dirty->max_dirty = true;
                            }
                            break;
                        case AST_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmin
                                    = node->compare_expr.value.float_value + __DBL_EPSILON__;
                                dirty->min_dirty = true;
                            }
                            else {
                                bound->fmax = node->compare_expr.value.float_value;
                                dirty->max_dirty = true;
                            }
                            break;
                        default:
                            switch_default_error("Invalid compare value type");
                            break;
                    }
                    break;
                case AST_COMPARE_GT:
                    switch(node->compare_expr.value.value_type) {
                        case AST_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imax = node->compare_expr.value.integer_value;
                                dirty->max_dirty = true;
                            }
                            else {
                                bound->imin = node->compare_expr.value.integer_value + 1;
                                dirty->min_dirty = true;
                            }
                            break;
                        case AST_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmax = node->compare_expr.value.float_value;
                                dirty->max_dirty = true;
                            }
                            else {
                                bound->fmin
                                    = node->compare_expr.value.float_value + __DBL_EPSILON__;
                                dirty->min_dirty = true;
                            }
                            break;
                        default:
                            switch_default_error("Invalid compare value type");
                            break;
                    }
                    break;
                case AST_COMPARE_GE:
                    switch(node->compare_expr.value.value_type) {
                        case AST_COMPARE_VALUE_INTEGER:
                            if(is_reversed) {
                                bound->imax = node->compare_expr.value.integer_value - 1;
                                dirty->max_dirty = true;
                            }
                            else {
                                bound->imin = node->compare_expr.value.integer_value;
                                dirty->min_dirty = true;
                            }
                            break;
                        case AST_COMPARE_VALUE_FLOAT:
                            if(is_reversed) {
                                bound->fmax
                                    = node->compare_expr.value.float_value - __DBL_EPSILON__;
                                dirty->max_dirty = true;
                            }
                            else {
                                bound->fmin = node->compare_expr.value.float_value;
                                dirty->min_dirty = true;
                            }
                            break;
                        default:
                            switch_default_error("Invalid compare value type");
                            break;
                    }
                    break;
                default:
                    switch_default_error("Invalid compare operation");
                    break;
                    ;
            }
            return;
        default:
            switch_default_error("Invalid expr type");
            return;
    }
}

struct value_bound get_variable_bound(const struct attr_domain* domain, const struct ast_node* node)
{
    struct value_bound bound = { .value_type = domain->bound.value_type };
    struct bound_dirty dirty = { .min_dirty = false, .max_dirty = false };
    get_variable_bound_inner(domain, node, &bound, false, &dirty);
    switch(domain->bound.value_type) {
        case BETREE_BOOLEAN:
            if(dirty.min_dirty == false) {
                bound.bmin = domain->bound.bmin;
            }
            if(dirty.max_dirty == false) {
                bound.bmax = domain->bound.bmax;
            }
            break;
        case BETREE_INTEGER:
        case BETREE_INTEGER_LIST:
            if(dirty.min_dirty == false) {
                bound.imin = domain->bound.imin;
            }
            if(dirty.max_dirty == false) {
                bound.imax = domain->bound.imax;
            }
            break;
        case BETREE_FLOAT:
            if(dirty.min_dirty == false) {
                bound.fmin = domain->bound.fmin;
            }
            if(dirty.max_dirty == false) {
                bound.fmax = domain->bound.fmax;
            }
            break;
        case BETREE_STRING:
        case BETREE_STRING_LIST:
        case BETREE_INTEGER_ENUM:
        case BETREE_INTEGER_LIST_ENUM:
            if(dirty.min_dirty == false) {
                bound.smin = domain->bound.smin;
            }
            if(dirty.max_dirty == false) {
                bound.smax = domain->bound.smax;
            }
            break;
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
        default:
            fprintf(stderr, "Invalid domain type to get a bound\n");
            abort();
    }
    return bound;
}

static bool get_integer_constant(
    const char* name, size_t count, const struct betree_constant** constants, int64_t* ret)
{
    const struct betree_constant* constant = NULL;
    for(size_t i = 0; i < count; i++) {
        if(strcmp(name, constants[i]->name) == 0) {
            constant = constants[i];
            break;
        }
    }
    if(constant == NULL || constant->value.value_type != BETREE_INTEGER) {
        return false;
    }
    *ret = constant->value.ivalue;
    return true;
}

static const char* get_constant_name_for_type(enum frequency_type_e type)
{
    switch(type) {
        case FREQUENCY_TYPE_ADVERTISER:
        case FREQUENCY_TYPE_ADVERTISERIP:
            return "advertiser_id";
        case FREQUENCY_TYPE_CAMPAIGN:
        case FREQUENCY_TYPE_CAMPAIGNIP:
            return "campaign_id";
        case FREQUENCY_TYPE_FLIGHT:
        case FREQUENCY_TYPE_FLIGHTIP:
            return "flight_id";
        case FREQUENCY_TYPE_PRODUCT:
        case FREQUENCY_TYPE_PRODUCTIP:
            return "product_id";
        default:
            return NULL;
    };
}

bool assign_constants(
    size_t constant_count, const struct betree_constant** constants, struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
        case AST_TYPE_COMPARE_EXPR:
        case AST_TYPE_EQUALITY_EXPR:
        case AST_TYPE_SET_EXPR:
        case AST_TYPE_LIST_EXPR:
            return true;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    bool lhs
                        = assign_constants(constant_count, constants, node->bool_expr.binary.lhs);
                    bool rhs
                        = assign_constants(constant_count, constants, node->bool_expr.binary.rhs);
                    return lhs && rhs;
                }
                case AST_BOOL_NOT:
                    return assign_constants(constant_count, constants, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                case AST_BOOL_LITERAL:
                default:
                    return true;
            }
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    const char* name
                        = get_constant_name_for_type(node->special_expr.frequency.type);
                    if(name == NULL) {
                        return false;
                    }
                    int64_t constant = INT64_MAX;
                    bool success = get_integer_constant(name, constant_count, constants, &constant);
                    if(success == false) {
                        return false;
                    }
                    node->special_expr.frequency.id = (uint32_t)constant;
                    return true;
                }
                case AST_SPECIAL_SEGMENT:
                case AST_SPECIAL_GEO:
                case AST_SPECIAL_STRING:
                    return true;
                default:
                    fprintf(stderr, "Invalid special node type\n");
                    abort();
            }
        default:
            fprintf(stderr, "Invalid node type\n");
            abort();
    }
}

void assign_variable_id(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR: {
            betree_var_t variable_id
                = try_get_id_for_attr(config, node->is_null_expr.attr_var.attr);
            node->is_null_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    betree_var_t variable_id
                        = try_get_id_for_attr(config, node->special_expr.frequency.attr_var.attr);
                    node->special_expr.frequency.attr_var.var = variable_id;
                    betree_var_t now_id
                        = try_get_id_for_attr(config, node->special_expr.frequency.now.attr);
                    node->special_expr.frequency.now.var = now_id;
                    return;
                }
                case AST_SPECIAL_SEGMENT: {
                    betree_var_t variable_id
                        = try_get_id_for_attr(config, node->special_expr.segment.attr_var.attr);
                    node->special_expr.segment.attr_var.var = variable_id;
                    betree_var_t now_id
                        = try_get_id_for_attr(config, node->special_expr.segment.now.attr);
                    node->special_expr.segment.now.var = now_id;
                    return;
                }
                case AST_SPECIAL_GEO: {
                    betree_var_t latitude_id
                        = try_get_id_for_attr(config, node->special_expr.geo.latitude_var.attr);
                    node->special_expr.geo.latitude_var.var = latitude_id;
                    betree_var_t longitude_id
                        = try_get_id_for_attr(config, node->special_expr.geo.longitude_var.attr);
                    node->special_expr.geo.longitude_var.var = longitude_id;
                    return;
                }
                case AST_SPECIAL_STRING: {
                    betree_var_t variable_id
                        = try_get_id_for_attr(config, node->special_expr.string.attr_var.attr);
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
        case(AST_TYPE_COMPARE_EXPR): {
            betree_var_t variable_id
                = try_get_id_for_attr(config, node->compare_expr.attr_var.attr);
            node->compare_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            betree_var_t variable_id
                = try_get_id_for_attr(config, node->equality_expr.attr_var.attr);
            node->equality_expr.attr_var.var = variable_id;
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL:
                    return;
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
                        = try_get_id_for_attr(config, node->bool_expr.variable.attr);
                    node->bool_expr.variable.var = variable_id;
                    return;
                }
                default:
                    switch_default_error("Invalid bool expr operation");
                    return;
            }
        }
        case(AST_TYPE_LIST_EXPR): {
            betree_var_t variable_id = try_get_id_for_attr(config, node->list_expr.attr_var.attr);
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
                    betree_var_t variable_id = try_get_id_for_attr(
                        config, node->set_expr.left_value.variable_value.attr);
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
                    betree_var_t variable_id = try_get_id_for_attr(
                        config, node->set_expr.right_value.variable_value.attr);
                    node->set_expr.right_value.variable_value.var = variable_id;
                    break;
                }
                case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
                    break;
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

void assign_ienum_id(struct config* config, struct ast_node* node, bool always_assign)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
        case AST_TYPE_SPECIAL_EXPR: 
        case AST_TYPE_COMPARE_EXPR: 
        case AST_TYPE_LIST_EXPR:
            return;
        case AST_TYPE_EQUALITY_EXPR:
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_INTEGER
              && config->attr_domains[node->equality_expr.attr_var.var]->bound.value_type == BETREE_INTEGER_ENUM) {
                betree_ienum_t ienum_id = get_id_for_ienum(config,
                    node->equality_expr.attr_var,
                    node->equality_expr.value.integer_value, always_assign);
                node->equality_expr.value.value_type = AST_EQUALITY_VALUE_INTEGER_ENUM;
                node->equality_expr.value.integer_enum_value.integer = node->equality_expr.value.integer_value;
                node->equality_expr.value.integer_enum_value.var = node->equality_expr.attr_var.var;
                node->equality_expr.value.integer_enum_value.ienum = ienum_id;
            }
            return;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL:
                    return;
                case AST_BOOL_NOT: {
                    assign_ienum_id(config, node->bool_expr.unary.expr, always_assign);
                    return;
                }
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    assign_ienum_id(config, node->bool_expr.binary.lhs, always_assign);
                    assign_ienum_id(config, node->bool_expr.binary.rhs, always_assign);
                    return;
                }
                case AST_BOOL_VARIABLE: {
                    return;
                }
                default:
                    switch_default_error("Invalid bool expr operation");
                    return;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
              && config->attr_domains[node->set_expr.left_value.variable_value.var]->bound.value_type == BETREE_INTEGER_ENUM) {
                struct betree_integer_list_enum* integer_list_enum = make_integer_list_enum(node->set_expr.right_value.integer_list_value->count);
                for(size_t i = 0; i < node->list_expr.value.string_list_value->count; i++) {
                    betree_ienum_t ienum_id = get_id_for_ienum(config,
                        node->set_expr.left_value.variable_value,
                        node->set_expr.right_value.integer_list_value->integers[i], always_assign);
                    integer_list_enum->integers[i].var = node->set_expr.left_value.variable_value.var;
                    integer_list_enum->integers[i].ienum = ienum_id;
                    integer_list_enum->integers[i].integer = node->set_expr.left_value.variable_value.var;
                }
                node->set_expr.right_value.value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM;
                free_integer_list(node->set_expr.right_value.integer_list_value);
                node->set_expr.right_value.integer_list_enum_value = integer_list_enum;

            }
            return;
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

void assign_str_id(struct config* config, struct ast_node* node, bool always_assign)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            return;
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    betree_str_t str_id = get_id_for_string(config,
                        node->special_expr.frequency.attr_var,
                        node->special_expr.frequency.ns.string, always_assign);
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
        case(AST_TYPE_COMPARE_EXPR): {
            return;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config,
                    node->equality_expr.attr_var,
                    node->equality_expr.value.string_value.string, always_assign);
                node->equality_expr.value.string_value.var = node->equality_expr.attr_var.var;
                node->equality_expr.value.string_value.str = str_id;
            }
            return;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL:
                    return;
                case AST_BOOL_NOT: {
                    assign_str_id(config, node->bool_expr.unary.expr, always_assign);
                    return;
                }
                case AST_BOOL_OR:
                case AST_BOOL_AND: {
                    assign_str_id(config, node->bool_expr.binary.lhs, always_assign);
                    assign_str_id(config, node->bool_expr.binary.rhs, always_assign);
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
                for(size_t i = 0; i < node->list_expr.value.string_list_value->count; i++) {
                    betree_str_t str_id = get_id_for_string(config,
                        node->list_expr.attr_var,
                        node->list_expr.value.string_list_value->strings[i].string, always_assign);
                    node->list_expr.value.string_list_value->strings[i].var
                        = node->list_expr.attr_var.var;
                    node->list_expr.value.string_list_value->strings[i].str = str_id;
                }
            }
            return;
        }
        case(AST_TYPE_SET_EXPR): {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                betree_str_t str_id = get_id_for_string(config,
                    node->set_expr.right_value.variable_value,
                    node->set_expr.left_value.string_value.string, always_assign);
                node->set_expr.left_value.string_value.var
                    = node->set_expr.right_value.variable_value.var;
                node->set_expr.left_value.string_value.str = str_id;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                for(size_t i = 0; i < node->set_expr.right_value.string_list_value->count; i++) {
                    betree_str_t str_id = get_id_for_string(config,
                        node->set_expr.left_value.variable_value,
                        node->set_expr.right_value.string_list_value->strings[i].string, always_assign);
                    node->set_expr.right_value.string_list_value->strings[i].var
                        = node->set_expr.left_value.variable_value.var;
                    node->set_expr.right_value.string_list_value->strings[i].str = str_id;
                }
            }
            return;
        }
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

static bool eq_compare_value(struct compare_value a, struct compare_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_COMPARE_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_COMPARE_VALUE_FLOAT:
            return feq(a.float_value, b.float_value);
        default:
            switch_default_error("Invalid compare value type");
            return false;
    }
}

static bool eq_equality_value(struct equality_value a, struct equality_value b)
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
            return a.string_value.var == b.string_value.var
                && a.string_value.str == b.string_value.str;
        case AST_EQUALITY_VALUE_INTEGER_ENUM:
            return a.integer_enum_value.var == b.integer_enum_value.var
                && a.integer_enum_value.ienum == b.integer_enum_value.ienum;
        default:
            switch_default_error("Invalid equality value type");
            return false;
    }
}

static bool eq_bool_expr(struct ast_bool_expr a, struct ast_bool_expr b)
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
        case AST_BOOL_LITERAL:
            return a.literal == b.literal;
        default:
            switch_default_error("Invalid bool expr op");
            return false;
    }
}

static bool eq_set_left_value(struct set_left_value a, struct set_left_value b)
{
    if(a.value_type != b.value_type) {
        return false;
    }
    switch(a.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            return a.integer_value == b.integer_value;
        case AST_SET_LEFT_VALUE_STRING:
            return a.string_value.var == b.string_value.var
                && a.string_value.str == b.string_value.str;
        case AST_SET_LEFT_VALUE_VARIABLE:
            return a.variable_value.var == b.variable_value.var;
        default:
            switch_default_error("Invalid left value type");
            return false;
    }
}

static bool eq_integer_list(struct betree_integer_list* a, struct betree_integer_list* b)
{
    if(a->count != b->count) {
        return false;
    }
    for(size_t i = 0; i < a->count; i++) {
        if(a->integers[i] != b->integers[i]) {
            return false;
        }
    }
    return true;
}

static bool eq_string_list(struct betree_string_list* a, struct betree_string_list* b)
{
    if(a->count != b->count) {
        return false;
    }
    for(size_t i = 0; i < a->count; i++) {
        if(a->strings[i].var == b->strings[i].var && a->strings[i].str != b->strings[i].str) {
            return false;
        }
    }
    return true;
}

static bool eq_integer_list_enum(struct betree_integer_list_enum* a, struct betree_integer_list_enum* b)
{
    if(a->count != b->count) {
        return false;
    }
    for(size_t i = 0; i < a->count; i++) {
        if(a->integers[i].var == b->integers[i].var && a->integers[i].ienum != b->integers[i].ienum) {
            return false;
        }
    }
    return true;
}

static bool eq_set_right_value(struct set_right_value a, struct set_right_value b)
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
        case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
            return eq_integer_list_enum(a.integer_list_enum_value, b.integer_list_enum_value);
        default:
            switch_default_error("Invalid right value type");
            return false;
    }
}

static bool eq_set_expr(struct ast_set_expr a, struct ast_set_expr b)
{
    if(a.op != b.op) {
        return false;
    }
    return eq_set_left_value(a.left_value, b.left_value)
        && eq_set_right_value(a.right_value, b.right_value);
}

static bool eq_list_value(struct list_value a, struct list_value b)
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

static bool eq_list_expr(struct ast_list_expr a, struct ast_list_expr b)
{
    if(a.op != b.op) {
        return false;
    }
    return a.attr_var.var == b.attr_var.var && eq_list_value(a.value, b.value);
}

static bool eq_special_expr(struct ast_special_expr a, struct ast_special_expr b)
{
    if(a.type != b.type) {
        return false;
    }
    switch(a.type) {
        case AST_SPECIAL_FREQUENCY:
            return a.frequency.attr_var.var == b.frequency.attr_var.var
                && a.frequency.length == b.frequency.length
                && a.frequency.ns.var == b.frequency.ns.var
                && a.frequency.ns.str == b.frequency.ns.str && a.frequency.op == b.frequency.op
                && a.frequency.type == b.frequency.type && a.frequency.value == b.frequency.value;
        case AST_SPECIAL_SEGMENT:
            return a.segment.attr_var.var == b.segment.attr_var.var
                && a.segment.has_variable == b.segment.has_variable && a.segment.op == b.segment.op
                && a.segment.seconds == b.segment.seconds
                && a.segment.segment_id == b.segment.segment_id;
        case AST_SPECIAL_GEO:
            return a.geo.has_radius == b.geo.has_radius && feq(a.geo.latitude, b.geo.latitude)
                && feq(a.geo.longitude, b.geo.longitude) && a.geo.op == b.geo.op
                && feq(a.geo.radius, b.geo.radius);
        case AST_SPECIAL_STRING:
            return a.string.attr_var.var == b.string.attr_var.var && a.string.op == b.string.op
                && strcmp(a.string.pattern, b.string.pattern) == 0;
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
        case AST_TYPE_IS_NULL_EXPR:
            return a->is_null_expr.attr_var.var == b->is_null_expr.attr_var.var;
        case AST_TYPE_COMPARE_EXPR: {
            return a->compare_expr.attr_var.var == b->compare_expr.attr_var.var
                && a->compare_expr.op == b->compare_expr.op
                && eq_compare_value(a->compare_expr.value, b->compare_expr.value);
        }
        case AST_TYPE_EQUALITY_EXPR: {
            return a->equality_expr.attr_var.var == b->equality_expr.attr_var.var
                && a->equality_expr.op == b->equality_expr.op
                && eq_equality_value(a->equality_expr.value, b->equality_expr.value);
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
            return a->bool_expr.binary.lhs->id == b->bool_expr.binary.lhs->id
                && a->bool_expr.binary.rhs->id == b->bool_expr.binary.rhs->id;
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
        case AST_TYPE_IS_NULL_EXPR:
        case AST_TYPE_COMPARE_EXPR:
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
                case AST_BOOL_LITERAL:
                    return;
                default:
                    switch_default_error("Invalid bool expr op");
                    return;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                switch(node->set_expr.right_value.value_type) {
                    case AST_SET_RIGHT_VALUE_INTEGER_LIST:
                        qsort(node->set_expr.right_value.integer_list_value->integers,
                            node->set_expr.right_value.integer_list_value->count,
                            sizeof(*node->set_expr.right_value.integer_list_value->integers),
                            icmpfunc);
                        return;
                    case AST_SET_RIGHT_VALUE_STRING_LIST:
                        qsort(node->set_expr.right_value.string_list_value->strings,
                            node->set_expr.right_value.string_list_value->count,
                            sizeof(*node->set_expr.right_value.string_list_value->strings),
                            scmpfunc);
                        return;
                    case AST_SET_RIGHT_VALUE_VARIABLE:
                    case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM:
                        qsort(node->set_expr.right_value.integer_list_enum_value->integers,
                            node->set_expr.right_value.integer_list_enum_value->count,
                            sizeof(*node->set_expr.right_value.integer_list_enum_value->integers),
                            iecmpfunc);
                        return;
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
                    qsort(node->list_expr.value.integer_list_value->integers,
                        node->list_expr.value.integer_list_value->count,
                        sizeof(*node->list_expr.value.integer_list_value->integers),
                        icmpfunc);
                    return;
                case AST_LIST_VALUE_STRING_LIST:
                    qsort(node->list_expr.value.string_list_value->strings,
                        node->list_expr.value.string_list_value->count,
                        sizeof(*node->list_expr.value.string_list_value->strings),
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
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        if(strcmp(attr, config->attr_domains[i]->attr_var.attr) == 0) {
            return true;
        }
    }
    return false;
}

bool all_variables_in_config(const struct config* config, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            return node->is_null_expr.attr_var.var != INVALID_VAR;
        case AST_TYPE_COMPARE_EXPR:
            return node->compare_expr.attr_var.var != INVALID_VAR;
        case AST_TYPE_EQUALITY_EXPR:
            return node->equality_expr.attr_var.var != INVALID_VAR;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    return all_variables_in_config(config, node->bool_expr.binary.lhs)
                        && all_variables_in_config(config, node->bool_expr.binary.rhs);
                case AST_BOOL_NOT:
                    return all_variables_in_config(config, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return node->bool_expr.variable.var != INVALID_VAR;
                case AST_BOOL_LITERAL:
                    return true;
                default:
                    switch_default_error("Invalid bool expr op");
                    return false;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                return node->set_expr.left_value.variable_value.var != INVALID_VAR;
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                return node->set_expr.right_value.variable_value.var != INVALID_VAR;
            }
            fprintf(stderr, "Invalid set expr");
            abort();
            return false;
        case AST_TYPE_LIST_EXPR:
            return node->list_expr.attr_var.var != INVALID_VAR;
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    return node->special_expr.frequency.attr_var.var != INVALID_VAR
                        && node->special_expr.frequency.now.var != INVALID_VAR;
                case AST_SPECIAL_SEGMENT:
                    return node->special_expr.segment.attr_var.var != INVALID_VAR
                        && node->special_expr.segment.now.var != INVALID_VAR;
                case AST_SPECIAL_GEO:
                    return true;
                case AST_SPECIAL_STRING:
                    return node->special_expr.string.attr_var.var != INVALID_VAR;
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
            size_t count = config->attr_domains[i]->bound.smax;
            if(count == SIZE_MAX) {
                return count;
            }
            else {
                return count + 1;
            }
        }
    }
    return SIZE_MAX;
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
    struct string_map* string_map = get_string_map_for_attr(config, attr);
    size_t space_left = string_map == NULL ? bound : bound - string_map->string_value_count;
    if(string_map != NULL) {
        for(size_t j = 0; j < string_map->string_value_count; j++) {
            if(strcmp(string, string_map->string_values[j]) == 0) {
                return true;
            }
        }
    }
    bool within_bound = space_left > 0;
    if(within_bound == false) {
        fprintf(stderr,
            "For attr '%s', string '%s' could not fit within the bound %zu\n",
            attr,
            string,
            bound);
    }
    return within_bound;
}

static bool strs_valid(
    const struct config* config, const char* attr, const struct betree_string_list* strings)
{
    size_t bound = get_attr_string_bound(config, attr);
    if(bound == SIZE_MAX) {
        return true;
    }
    struct string_map* string_map = get_string_map_for_attr(config, attr);
    size_t space_left = string_map == NULL ? bound : bound - string_map->string_value_count;
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
        fprintf(stderr,
            "For attr '%s', not all strings could not fit within the bound %zu\n",
            attr,
            bound);
    }
    return within_bound;
}

bool all_bounded_strings_valid(const struct config* config, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
        case AST_TYPE_COMPARE_EXPR:
            return true;
        case AST_TYPE_EQUALITY_EXPR:
            if(node->equality_expr.value.value_type == AST_EQUALITY_VALUE_STRING) {
                return str_valid(config,
                    node->equality_expr.attr_var.attr,
                    node->equality_expr.value.string_value.string);
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
                case AST_BOOL_LITERAL:
                    return true;
                default:
                    switch_default_error("Invalid bool expr op");
                    return false;
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE
                && node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
                return strs_valid(config,
                    node->set_expr.left_value.variable_value.attr,
                    node->set_expr.right_value.string_list_value);
            }
            if(node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE
                && node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_STRING) {
                return str_valid(config,
                    node->set_expr.right_value.variable_value.attr,
                    node->set_expr.left_value.string_value.string);
            }
            return true;
        case AST_TYPE_LIST_EXPR:
            if(node->list_expr.value.value_type == AST_LIST_VALUE_STRING_LIST) {
                return strs_valid(
                    config, node->list_expr.attr_var.attr, node->list_expr.value.string_list_value);
            }
            return true;
        case AST_TYPE_SPECIAL_EXPR:
            return true;
        default:
            switch_default_error("Invalid node type");
            return false;
    }
}
