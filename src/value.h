#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "betree.h"

typedef uint64_t betree_var_t;
static const betree_var_t INVALID_VAR = UINT64_MAX;
typedef uint64_t betree_str_t;
static const betree_str_t INVALID_STR = UINT64_MAX;
typedef uint64_t betree_seg_t;

typedef uint64_t betree_ienum_t;
static const betree_ienum_t INVALID_IENUM = UINT64_MAX;

struct string_value {
    const char* string;
    betree_var_t var;
    betree_str_t str;
};

struct integer_enum_value {
    int64_t integer;
    betree_var_t var;
    betree_ienum_t ienum;
};

struct betree_integer_list {
    size_t count;
    int64_t* integers;
};

struct betree_integer_enum_list {
    size_t count;
    struct integer_enum_value* integers;
};

struct betree_string_list {
    size_t count;
    struct string_value* strings;
};

struct betree_segment {
    int64_t id;
    int64_t timestamp;
};

struct betree_segments {
    size_t size;
    struct betree_segment** content;
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

struct betree_frequency_cap {
    enum frequency_type_e type;
    uint32_t id;
    struct string_value namespace;
    bool timestamp_defined;
    int64_t timestamp;
    uint32_t value;
};

struct betree_frequency_caps {
    size_t size;
    struct betree_frequency_cap** content;
};

struct value {
    enum betree_value_type_e value_type;
    union {
        int64_t integer_value;
        double float_value;
        bool boolean_value;
        struct string_value string_value;
        struct betree_integer_list* integer_list_value;
        struct betree_string_list* string_list_value;
        struct betree_segments* segments_value;
        struct betree_frequency_caps* frequency_caps_value;
        struct integer_enum_value integer_enum_value;
        struct betree_integer_enum_list* integer_enum_list_value;
    };
};

struct value_bound {
    enum betree_value_type_e value_type;
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

struct betree_integer_list* make_integer_list();
struct betree_string_list* make_string_list();
struct betree_segments* make_segments();
struct betree_frequency_caps* make_frequency_caps();
struct betree_integer_enum_list* make_integer_enum_list(size_t count);

void add_integer_list_value(int64_t integer, struct betree_integer_list* list);
char* integer_list_value_to_string(struct betree_integer_list* list);
void add_string_list_value(struct string_value string, struct betree_string_list* list);
char* string_list_value_to_string(struct betree_string_list* list);
char* integer_enum_list_value_to_string(struct betree_integer_enum_list* list);
void add_segment(struct betree_segment* segment, struct betree_segments* list);
void add_frequency(struct betree_frequency_cap* frequency, struct betree_frequency_caps* list);
struct betree_segment* make_segment(int64_t id, int64_t timestamp);
struct betree_frequency_cap* make_frequency_cap(const char* stype,
    uint32_t id,
    struct string_value namespace,
    bool timestamp_defined,
    int64_t timestamp,
    uint32_t value);

enum frequency_type_e get_type_from_string(const char* stype);
char* segments_value_to_string(struct betree_segments* list);
char* frequency_caps_value_to_string(struct betree_frequency_caps* list);

void free_integer_list(struct betree_integer_list* value);
void free_string_list(struct betree_string_list* value);
void free_segment(struct betree_segment* value);
void free_segments(struct betree_segments* value);
void free_frequency_cap(struct betree_frequency_cap* value);
void free_frequency_caps(struct betree_frequency_caps* value);
void free_integer_enum_list(struct betree_integer_enum_list* value);

void free_value(struct value value);

void remove_duplicates_integer_list(struct betree_integer_list* list);
void sort_integer_list(struct betree_integer_list* list);
void sort_and_remove_duplicate_integer_list(struct betree_integer_list* list);
void remove_duplicates_string_list(struct betree_string_list* list);
void sort_string_list(struct betree_string_list* list);
void sort_and_remove_duplicate_string_list(struct betree_string_list* list);
void remove_duplicates_integer_enum_list(struct betree_integer_enum_list* list);
void sort_integer_enum_list(struct betree_integer_enum_list* list);
void sort_and_remove_duplicate_integer_enum_list(struct betree_integer_enum_list* list);

