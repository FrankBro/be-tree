#pragma once

#include <stdbool.h>

#include "betree.h"
#include "config.h"
#include "memoize.h"
#include "value.h"
#include "var.h"

// Comparison (<, <=, >, >=)
// Work on integer and float

enum ast_compare_e {
    AST_COMPARE_LT,
    AST_COMPARE_LE,
    AST_COMPARE_GT,
    AST_COMPARE_GE,
};

enum ast_compare_value_e {
    AST_COMPARE_VALUE_INTEGER,
    AST_COMPARE_VALUE_FLOAT,
};

struct compare_value {
    enum ast_compare_value_e value_type;
    union {
        int64_t integer_value;
        double float_value;
    };
};

struct ast_compare_expr {
    enum ast_compare_e op;
    struct attr_var attr_var;
    struct compare_value value;
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
    AST_EQUALITY_VALUE_INTEGER_ENUM,
};

struct equality_value {
    enum ast_equality_value_e value_type;
    union {
        int64_t integer_value;
        double float_value;
        struct string_value string_value;
        struct integer_enum_value integer_enum_value;
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
    AST_BOOL_LITERAL,
};

struct ast_node;

struct ast_bool_binary {
    struct ast_node* lhs;
    struct ast_node* rhs;
};

struct ast_bool_unary {
    struct ast_node* expr;
};

struct ast_bool_expr {
    enum ast_bool_e op;
    union {
        struct ast_bool_binary binary;
        struct ast_bool_unary unary;
        struct attr_var variable;
        bool literal;
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
    AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM,
};

struct set_right_value {
    enum set_right_value_e value_type;
    union {
        struct betree_integer_list* integer_list_value;
        struct betree_string_list* string_list_value;
        struct attr_var variable_value;
        struct betree_integer_enum_list* integer_enum_list_value;
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
        struct betree_integer_list* integer_list_value;
        struct betree_string_list* string_list_value;
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
    struct attr_var now;
    uint32_t id;
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
    struct attr_var now;
};

enum ast_special_geo_e {
    AST_SPECIAL_GEOWITHINRADIUS,
};

struct ast_special_geo {
    enum ast_special_geo_e op;
    bool has_radius;
    double latitude;
    double longitude;
    double radius;
    struct attr_var latitude_var;
    struct attr_var longitude_var;
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

// Is null

enum ast_is_null_e {
    AST_IS_NULL,
    AST_IS_NOT_NULL,
    AST_IS_EMPTY,
};

struct ast_is_null_expr {
    enum ast_is_null_e type;
    struct attr_var attr_var;
};

// Expression

enum ast_node_type_e {
    AST_TYPE_COMPARE_EXPR,
    AST_TYPE_EQUALITY_EXPR,
    AST_TYPE_BOOL_EXPR,
    AST_TYPE_SET_EXPR,
    AST_TYPE_LIST_EXPR,
    AST_TYPE_SPECIAL_EXPR,
    AST_TYPE_IS_NULL_EXPR,
};

struct ast_node {
    betree_pred_t global_id;
    betree_pred_t memoize_id;
    enum ast_node_type_e type;
    union {
        struct ast_compare_expr compare_expr;
        struct ast_equality_expr equality_expr;
        struct ast_bool_expr bool_expr;
        struct ast_set_expr set_expr;
        struct ast_list_expr list_expr;
        struct ast_special_expr special_expr;
        struct ast_is_null_expr is_null_expr;
    };
};

struct ast_node* ast_node_create();
struct ast_node* ast_compare_expr_create(
    enum ast_compare_e op, const char* name, struct compare_value value);
struct ast_node* ast_equality_expr_create(
    enum ast_equality_e op, const char* name, struct equality_value value);
struct ast_node* ast_set_expr_create(
    enum ast_set_e op, struct set_left_value left_value, struct set_right_value right_value);
struct ast_node* ast_list_expr_create(
    enum ast_list_e op, const char* name, struct list_value list_value);
struct ast_node* ast_bool_expr_literal_create(bool value);
struct ast_node* ast_bool_expr_variable_create(const char* name);
struct ast_node* ast_bool_expr_unary_create(struct ast_node* expr);
struct ast_node* ast_bool_expr_binary_create(
    enum ast_bool_e op, struct ast_node* lhs, struct ast_node* rhs);

struct ast_node* ast_special_frequency_create(enum ast_special_frequency_e op,
    const char* type,
    struct string_value ns,
    int64_t value,
    size_t length);
struct ast_node* ast_special_segment_create(
    enum ast_special_segment_e op, const char* name, betree_seg_t segment_id, int64_t seconds);
struct ast_node* ast_special_geo_create(
    enum ast_special_geo_e op, double latitude, double longitude, bool has_radius, double radius);
struct ast_node* ast_special_string_create(
    enum ast_special_string_e op, const char* name, const char* pattern);

struct ast_node* ast_is_null_expr_create(enum ast_is_null_e type, const char* name);

void free_ast_node(struct ast_node* node);

bool match_node(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report* report);

struct value_bound get_variable_bound(
    const struct attr_domain* domain, const struct ast_node* node);

bool assign_constants(
    size_t constant_count, const struct betree_constant** constants, struct ast_node* node);
void assign_variable_id(struct config* config, struct ast_node* node);
void assign_str_id(struct config* config, struct ast_node* node, bool always_assign);
void assign_ienum_id(struct config* config, struct ast_node* node, bool always_assign);
void assign_pred_id(struct config* config, struct ast_node* node);
void sort_lists(struct ast_node* node);

const char* frequency_type_to_string(enum frequency_type_e type);
bool eq_expr(const struct ast_node* a, const struct ast_node* b);
bool fast_eq_expr(const struct ast_node* a, const struct ast_node* b);

bool all_variables_in_config(const struct config* config, const struct ast_node* node);
bool all_bounded_strings_valid(const struct config* config, const struct ast_node* node);
