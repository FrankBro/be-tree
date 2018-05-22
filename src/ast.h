#pragma once

#include <stdbool.h>

#include "betree.h"

// Numeric comparison (<, <=, >, >=)
// Work on integer and float

enum ast_numeric_compare_e {
    AST_NUMERIC_COMPARE_LT,
    AST_NUMERIC_COMPARE_LE,
    AST_NUMERIC_COMPARE_GT,
    AST_NUMERIC_COMPARE_GE,
};

enum ast_numeric_compare_value_e {
    AST_NUMERIC_COMPARE_VALUE_INTEGER,
    AST_NUMERIC_COMPARE_VALUE_FLOAT,
};

struct numeric_compare_value {
    enum ast_numeric_compare_value_e value_type;
    union {
        int64_t integer_value;
        double float_value;
    };
};

struct ast_numeric_compare_expr {
    enum ast_numeric_compare_e op;
    struct attr_var attr_var;
    struct numeric_compare_value value;
};

// Equality (=, <>)
// Work on integer, float and string

enum ast_equality_e {
    AST_EQUALITY_EQ,
    AST_EQUALITY_NE,
};

enum ast_equality_value_e {
    AST_EQUALITY_VALUE_INTEGER,
    AST_EQUALITY_VALUE_FLOAT,
    AST_EQUALITY_VALUE_STRING,
};

struct equality_value {
    enum ast_equality_value_e value_type;
    union {
        int64_t integer_value;
        double float_value;
        struct string_value string_value;
    };
};

struct ast_equality_expr {
    enum ast_equality_e op;
    struct attr_var attr_var;
    struct equality_value value;
};

// Bool
// Binary (or, and)
// Unary (not)
// Variable

enum ast_bool_e {
    AST_BOOL_OR,
    AST_BOOL_AND,
    AST_BOOL_NOT,
    AST_BOOL_VARIABLE,
};

struct ast_node;

struct ast_bool_binary {
    const struct ast_node* lhs;
    const struct ast_node* rhs;
};

struct ast_bool_unary {
    const struct ast_node* expr;
};

struct ast_bool_expr {
    enum ast_bool_e op;
    union {
        struct ast_bool_binary binary;
        struct ast_bool_unary unary;
        struct attr_var variable;
    };
};

// Set ('in'/'not in')
// Left side can be integer, string or variable representing the previous types
// Right side can be integer list, string list or variable representing the previous types

enum set_left_value_e {
    AST_SET_LEFT_VALUE_INTEGER,
    AST_SET_LEFT_VALUE_STRING,
    AST_SET_LEFT_VALUE_VARIABLE,
};

struct set_left_value {
    enum set_left_value_e value_type;
    union {
        int64_t integer_value;
        struct string_value string_value;
        struct attr_var variable_value;
    };
};

enum set_right_value_e {
    AST_SET_RIGHT_VALUE_INTEGER_LIST,
    AST_SET_RIGHT_VALUE_STRING_LIST,
    AST_SET_RIGHT_VALUE_VARIABLE,
};

struct set_right_value {
    enum set_right_value_e value_type;
    union {
        struct integer_list_value integer_list_value;
        struct string_list_value string_list_value;
        struct attr_var variable_value;
    };
};

enum ast_set_e {
    AST_SET_NOT_IN,
    AST_SET_IN,
};

struct ast_set_expr {
    enum ast_set_e op;
    struct set_left_value left_value;
    struct set_right_value right_value;
};

// List ('one of'/'none of'/'all of')

enum ast_list_e {
    AST_LIST_ONE_OF,
    AST_LIST_NONE_OF,
    AST_LIST_ALL_OF,
};

enum ast_list_value_e {
    AST_LIST_VALUE_INTEGER_LIST,
    AST_LIST_VALUE_STRING_LIST,
};

struct list_value {
    enum ast_list_value_e value_type;
    union {
        struct integer_list_value integer_list_value;
        struct string_list_value string_list_value;
    };
};

struct ast_list_expr {
    enum ast_list_e op;
    struct attr_var attr_var;
    struct list_value value;
};

// Special

enum ast_special_frequency_e {
    AST_SPECIAL_WITHINFREQUENCYCAP,
};

struct ast_special_frequency {
    enum ast_special_frequency_e op;
    struct attr_var attr_var;
    enum frequency_type_e type;
    struct string_value ns;
    int64_t value;
    size_t length;
};

enum ast_special_segment_e {
    AST_SPECIAL_SEGMENTWITHIN,
    AST_SPECIAL_SEGMENTBEFORE,
};

struct ast_special_segment {
    enum ast_special_segment_e op;
    bool has_variable;
    struct attr_var attr_var;
    betree_seg_t segment_id;
    int64_t seconds;
};

enum ast_special_geo_value_e {
    AST_SPECIAL_GEO_VALUE_INTEGER,
    AST_SPECIAL_GEO_VALUE_FLOAT,
};

struct special_geo_value {
    enum ast_special_geo_value_e value_type;
    union {
        int64_t integer_value;
        double float_value;
    };
};

enum ast_special_geo_e {
    AST_SPECIAL_GEOWITHINRADIUS,
};

struct ast_special_geo {
    enum ast_special_geo_e op;
    bool has_radius;
    struct special_geo_value latitude;
    struct special_geo_value longitude;
    struct special_geo_value radius;
};

enum ast_special_string_e {
    AST_SPECIAL_CONTAINS,
    AST_SPECIAL_STARTSWITH,
    AST_SPECIAL_ENDSWITH,
};

struct ast_special_string {
    enum ast_special_string_e op;
    struct attr_var attr_var;
    const char* pattern;
};

enum ast_special_e {
    AST_SPECIAL_FREQUENCY,
    AST_SPECIAL_SEGMENT,
    AST_SPECIAL_GEO,
    AST_SPECIAL_STRING,
};

struct ast_special_expr {
    enum ast_special_e type;
    union {
        struct ast_special_frequency frequency;
        struct ast_special_segment segment;
        struct ast_special_geo geo;
        struct ast_special_string string;
    };
};

// Expression

enum ast_node_type_e {
    AST_TYPE_NUMERIC_COMPARE_EXPR,
    AST_TYPE_EQUALITY_EXPR,
    AST_TYPE_BOOL_EXPR,
    AST_TYPE_SET_EXPR,
    AST_TYPE_LIST_EXPR,
    AST_TYPE_SPECIAL_EXPR,
};

struct ast_node {
    enum ast_node_type_e type;
    union {
        struct ast_numeric_compare_expr numeric_compare_expr;
        struct ast_equality_expr equality_expr;
        struct ast_bool_expr bool_expr;
        struct ast_set_expr set_expr;
        struct ast_list_expr list_expr;
        struct ast_special_expr special_expr;
    };
};

struct ast_node* ast_numeric_compare_expr_create(
    const enum ast_numeric_compare_e op, const char* name, struct numeric_compare_value value);
struct ast_node* ast_equality_expr_create(
    const enum ast_equality_e op, const char* name, struct equality_value value);
struct ast_node* ast_set_expr_create(
    const enum ast_set_e op, struct set_left_value left_value, struct set_right_value right_value);
struct ast_node* ast_list_expr_create(
    const enum ast_list_e op, const char* name, struct list_value list_value);
struct ast_node* ast_bool_expr_variable_create(const char* name);
struct ast_node* ast_bool_expr_unary_create(const struct ast_node* expr);
struct ast_node* ast_bool_expr_binary_create(
    const enum ast_bool_e op, const struct ast_node* lhs, const struct ast_node* rhs);

struct ast_node* ast_special_frequency_create(const enum ast_special_frequency_e op,
    const char* type,
    struct string_value ns,
    int64_t value,
    size_t length);
struct ast_node* ast_special_segment_create(const enum ast_special_segment_e op,
    const char* name,
    betree_seg_t segment_id,
    int64_t seconds);
struct ast_node* ast_special_geo_create(const enum ast_special_geo_e op,
    struct special_geo_value latitude,
    struct special_geo_value longitude,
    bool has_radius,
    struct special_geo_value radius);
struct ast_node* ast_special_string_create(
    const enum ast_special_string_e op, const char* name, const char* pattern);
void free_ast_node(struct ast_node* node);

bool match_node(struct config* config, const struct event* event, const struct ast_node* node);

void get_variable_bound(
    const struct attr_domain* domain, const struct ast_node* node, struct value_bound* bound);

void assign_variable_id(struct config* config, struct ast_node* node);
void assign_str_id(struct config* config, struct ast_node* node);

// const char* ast_to_string(const struct ast_node* node);
struct string_value frequency_type_to_string(struct config* config, enum frequency_type_e type);
bool eq_expr(const struct ast_node* a, const struct ast_node* b);


