#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum value_e {
    VALUE_B,
    VALUE_I,
    VALUE_F,
    VALUE_S,
    VALUE_IL,
    VALUE_SL,
    VALUE_SEGMENTS,
    VALUE_FREQUENCY,
};

typedef uint64_t betree_var_t;
static const betree_var_t INVALID_VAR = UINT64_MAX;
typedef uint64_t betree_str_t;
static const betree_str_t INVALID_STR = UINT64_MAX;
typedef uint64_t betree_seg_t;

struct string_value {
    const char* string;
    betree_var_t var;
    betree_str_t str;
};

struct integer_list_value {
    size_t count;
    int64_t* integers;
};

struct string_list_value {
    size_t count;
    struct string_value* strings;
};

struct segment {
    int64_t id;
    int64_t timestamp;
};

struct segments_list {
    size_t size;
    struct segment* content;
};

enum frequency_type_e {
    FREQUENCY_TYPE_ADVERTISER,
    FREQUENCY_TYPE_ADVERTISERIP,
    FREQUENCY_TYPE_CAMPAIGN,
    FREQUENCY_TYPE_CAMPAIGNIP,
    FREQUENCY_TYPE_FLIGHT,
    FREQUENCY_TYPE_FLIGHTIP,
    FREQUENCY_TYPE_PRODUCT,
    FREQUENCY_TYPE_PRODUCTIP,
};

struct frequency_cap {
    enum frequency_type_e type;
    uint32_t id;
    struct string_value namespace;
    bool timestamp_defined;
    int64_t timestamp;
    uint32_t value;
};

struct frequency_caps_list {
    size_t size;
    struct frequency_cap* content;
};

struct value {
    enum value_e value_type;
    union {
        int64_t ivalue;
        double fvalue;
        bool bvalue;
        struct string_value svalue;
        struct integer_list_value ilvalue;
        struct string_list_value slvalue;
        struct segments_list segments_value;
        struct frequency_caps_list frequency_value;
    };
};

struct value_bound {
    enum value_e value_type;
    union {
        struct {
            int64_t imin;
            int64_t imax;
        };
        struct {
            double fmin;
            double fmax;
        };
        struct {
            bool bmin;
            bool bmax;
        };
        struct {
            size_t smin;
            size_t smax;
        };
    };
};

void add_integer_list_value(int64_t integer, struct integer_list_value* list);
const char* integer_list_value_to_string(struct integer_list_value list);
void add_string_list_value(struct string_value string, struct string_list_value* list);
const char* string_list_value_to_string(struct string_list_value list);
void add_segment(struct segment segment, struct segments_list* list);
void add_frequency(struct frequency_cap frequency, struct frequency_caps_list* list);
struct segment make_segment(int64_t id, int64_t timestamp);
struct frequency_cap make_frequency_cap(const char* type,
    uint32_t id,
    struct string_value namespace,
    int64_t timestamp,
    uint32_t value);

enum frequency_type_e get_type_from_string(const char* stype);

