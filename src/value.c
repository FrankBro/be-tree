#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "utils.h"
#include "value.h"

struct betree_integer_list* make_integer_list()
{
    struct betree_integer_list* value = bcalloc(sizeof(*value));
    if(value == NULL) {
        fprintf(stderr, "%s bcalloc failed", __func__);
        abort();
    }
    return value;
}

struct betree_integer_enum_list* make_integer_enum_list(size_t count)
{
    struct betree_integer_enum_list* value = bcalloc(sizeof(*value));
    if(value == NULL) {
        fprintf(stderr, "%s bcalloc failed", __func__);
        abort();
    }
    value->count = count;
    value->integers = bcalloc(count * sizeof(*value->integers));
    return value;
}

struct betree_string_list* make_string_list()
{
    struct betree_string_list* value = bcalloc(sizeof(*value));
    if(value == NULL) {
        fprintf(stderr, "%s bcalloc failed", __func__);
        abort();
    }
    return value;
}

struct betree_segments* make_segments()
{
    struct betree_segments* value = bcalloc(sizeof(*value));
    if(value == NULL) {
        fprintf(stderr, "%s bcalloc failed", __func__);
        abort();
    }
    return value;
}

struct betree_frequency_caps* make_frequency_caps()
{
    struct betree_frequency_caps* value = bcalloc(sizeof(*value));
    if(value == NULL) {
        fprintf(stderr, "%s bcalloc failed", __func__);
        abort();
    }
    return value;
}

void add_integer_list_value(int64_t integer, struct betree_integer_list* list)
{
    if(list->count == 0) {
        list->integers = bcalloc(sizeof(*list->integers));
        if(list->integers == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        int64_t* integers = brealloc(list->integers, sizeof(*list->integers) * (list->count + 1));
        if(integers == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        list->integers = integers;
    }
    list->integers[list->count] = integer;
    list->count++;
}

char* integer_list_value_to_string(struct betree_integer_list* list)
{
    char* string = NULL;
    for(size_t i = 0; i < list->count; i++) {
        char* new_string;
        if(i != 0) {
            if(basprintf(&new_string, "%s, %ld", string, list->integers[i]) < 0) {
                abort();
            }
            bfree(string);
        }
        else {
            if(basprintf(&new_string, "%ld", list->integers[i]) < 0) {
                abort();
            }
        }
        string = new_string;
    }
    return string;
}

void add_string_list_value(struct string_value string, struct betree_string_list* list)
{
    if(list->count == 0) {
        list->strings = bcalloc(sizeof(*list->strings));
        if(list->strings == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        struct string_value* strings
            = brealloc(list->strings, sizeof(*list->strings) * (list->count + 1));
        if(strings == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        list->strings = strings;
    }
    list->strings[list->count] = string;
    list->count++;
}

char* string_list_value_to_string(struct betree_string_list* list)
{
    char* string = NULL;
    for(size_t i = 0; i < list->count; i++) {
        char* new_string;
        if(i != 0) {
            if(basprintf(&new_string, "%s, \"%s\"", string, list->strings[i].string) < 0) {
                abort();
            }
            bfree(string);
        }
        else {
            if(basprintf(&new_string, "\"%s\"", list->strings[i].string) < 0) {
                abort();
            }
        }
        string = new_string;
    }
    return string;
}

char* integer_enum_list_value_to_string(struct betree_integer_enum_list* list)
{
    char* string = NULL;
    for(size_t i = 0; i < list->count; i++) {
        char* new_string;
        if(i != 0) {
            if(basprintf(&new_string, "%s, %ld", string, list->integers[i].integer) < 0) {
                abort();
            }
            bfree(string);
        }
        else {
            if(basprintf(&new_string, "%ld", list->integers[i].integer) < 0) {
                abort();
            }
        }
        string = new_string;
    }
    return string;
}

void add_segment(struct betree_segment* segment, struct betree_segments* list)
{
    if(list->size == 0) {
        list->content = bcalloc(sizeof(*list->content));
        if(list->content == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        struct betree_segment** content = brealloc(list->content, sizeof(*list->content) * (list->size + 1));
        if(content == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        list->content = content;
    }
    list->content[list->size] = segment;
    list->size++;
}

void add_frequency(struct betree_frequency_cap* frequency, struct betree_frequency_caps* list)
{
    if(list->size == 0) {
        list->content = bcalloc(sizeof(*list->content));
        if(list->content == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        struct betree_frequency_cap** content
            = brealloc(list->content, sizeof(*list->content) * (list->size + 1));
        if(content == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        list->content = content;
    }
    list->content[list->size] = frequency;
    list->size++;
}

struct betree_segment* make_segment(int64_t id, int64_t timestamp)
{
    struct betree_segment* segment = bmalloc(sizeof(*segment));
    segment->id = id;
    segment->timestamp = timestamp;
    return segment;
}

struct betree_frequency_cap* make_frequency_cap(const char* stype,
    uint32_t id,
    struct string_value namespace,
    bool timestamp_defined,
    int64_t timestamp,
    uint32_t value)
{
    struct betree_frequency_cap* frequency_cap = bmalloc(sizeof(*frequency_cap));
    enum frequency_type_e type = get_type_from_string(stype);
    frequency_cap->type = type;
    frequency_cap->id = id;
    frequency_cap->namespace = namespace;
    frequency_cap->timestamp_defined = timestamp_defined;
    frequency_cap->timestamp = timestamp;
    frequency_cap->value = value;
    return frequency_cap;
}

enum frequency_type_e get_type_from_string(const char* stype)
{
    if(strcmp(stype, "advertiser") == 0) {
        return FREQUENCY_TYPE_ADVERTISER;
    }
    if(strcmp(stype, "advertiser:ip") == 0) {
        return FREQUENCY_TYPE_ADVERTISERIP;
    }
    if(strcmp(stype, "campaign") == 0) {
        return FREQUENCY_TYPE_CAMPAIGN;
    }
    if(strcmp(stype, "campaign:ip") == 0) {
        return FREQUENCY_TYPE_CAMPAIGNIP;
    }
    if(strcmp(stype, "product") == 0) {
        return FREQUENCY_TYPE_PRODUCT;
    }
    if(strcmp(stype, "product:ip") == 0) {
        return FREQUENCY_TYPE_PRODUCTIP;
    }
    if(strcmp(stype, "flight") == 0) {
        return FREQUENCY_TYPE_FLIGHT;
    }
    if(strcmp(stype, "flight:ip") == 0) {
        return FREQUENCY_TYPE_FLIGHTIP;
    }
    fprintf(stderr, "Invalid frequency type");
    abort();
}

void free_integer_list(struct betree_integer_list* value)
{
    bfree(value->integers);
    bfree(value);
}

void free_integer_enum_list(struct betree_integer_enum_list* value)
{
    bfree(value->integers);
    bfree(value);
}

void free_string_list(struct betree_string_list* value)
{
    for(size_t i = 0; i < value->count; i++) {
        bfree((char*)value->strings[i].string);
    }
    bfree(value->strings);
    bfree(value);
}

void free_segment(struct betree_segment* value)
{
    bfree(value);
}

void free_segments(struct betree_segments* value)
{
    for(size_t i = 0; i < value->size; i++) {
        if(value->content[i] != NULL) {
            bfree(value->content[i]);
        }
    }
    bfree(value->content);
    bfree(value);
}

void free_frequency_cap(struct betree_frequency_cap* value)
{
    bfree((char*)value->namespace.string);
    bfree(value);
}

void free_frequency_caps(struct betree_frequency_caps* value)
{
    for(size_t i = 0; i < value->size; i++) {
        if(value->content[i] != NULL) {
            free_frequency_cap(value->content[i]);
        }
    }
    bfree(value->content);
    bfree(value);
}

void free_value(struct value value)
{
    switch(value.value_type) {
        case BETREE_INTEGER_LIST: {
            free_integer_list(value.integer_list_value);
            break;
        }
        case BETREE_INTEGER_LIST_ENUM:
            free_integer_enum_list(value.integer_enum_list_value);
            break;
        case BETREE_STRING_LIST: {
            free_string_list(value.string_list_value);
            break;
        }
        case BETREE_STRING: {
            bfree((char*)value.string_value.string);
        }
        case BETREE_BOOLEAN:
        case BETREE_INTEGER:
        case BETREE_INTEGER_ENUM:
        case BETREE_FLOAT: {
            break;
        }
        case BETREE_SEGMENTS: {
            free_segments(value.segments_value);
            break;
        }
        case BETREE_FREQUENCY_CAPS: {
            free_frequency_caps(value.frequency_caps_value);
            break;
        }
        default: abort();
    }
}

char* segment_value_to_string(struct betree_segment* segment)
{
    char* string = NULL;
    if(basprintf(&string, "[%ld, %ld]", segment->id, segment->timestamp) < 0) {
        abort();
    }
    return string;
}

char* segments_value_to_string(struct betree_segments* list)
{
    char* string = NULL;
    for(size_t i = 0; i < list->size; i++) {
        char* new_string;
        char* segment = segment_value_to_string(list->content[i]);
        if(i != 0) {
            if(basprintf(&new_string, "%s, %s", string, segment) < 0) {
                abort();
            }
            bfree(string);
        }
        else {
            if(basprintf(&new_string, "%s", segment) < 0) {
                abort();
            }
        }
        bfree(segment);
        string = new_string;
    }
    return string;
}

char* frequency_cap_to_string(struct betree_frequency_cap* cap)
{
    char* string = NULL;
    const char* type = frequency_type_to_string(cap->type);
    if(cap->timestamp_defined) {
        if(basprintf(&string, "[[\"%s\", %u, \"%s\"], %u, %ld]", type, cap->id, cap->namespace.string, cap->value, cap->timestamp) < 0) {
            abort();
        }
    }
    else {
        if(basprintf(&string, "[[\"%s\", %u, \"%s\"], %u, undefined]", type, cap->id, cap->namespace.string, cap->value) < 0) {
            abort();
        }
    }
    return string;
}

char* frequency_caps_value_to_string(struct betree_frequency_caps* list)
{
    char* string = NULL;
    for(size_t i = 0; i < list->size; i++) {
        char* new_string;
        char* cap = frequency_cap_to_string(list->content[i]);
        if(i != 0) {
            if(basprintf(&new_string, "%s, %s", string, cap) < 0) {
                abort();
            }
            bfree(string);
        }
        else {
            if(basprintf(&new_string, "%s", cap) < 0) {
                abort();
            }
        }
        bfree(cap);
        string = new_string;
    }
    return string;
}

void remove_duplicates_integer_list(struct betree_integer_list* list)
{
    if (list->count == 0) {
        return;
    }
    size_t r = 0;
    for (size_t i = 1; i < list->count; i++) {
        if (list->integers[r] != list->integers[i]) {
            list->integers[++ r] = list->integers[i]; // copy-in next unique number
        }
    }
    list->count = r + 1;
}

void sort_integer_list(struct betree_integer_list* list)
{
    qsort(list->integers, list->count, sizeof(*list->integers), icmpfunc);
}

void sort_and_remove_duplicate_integer_list(struct betree_integer_list* list)
{
    sort_integer_list(list);
    remove_duplicates_integer_list(list);
}

void remove_duplicates_string_list(struct betree_string_list* list)
{
    if (list->count == 0) {
        return;
    }
    size_t r = 0;
    for (size_t i = 1; i < list->count; i++) {
        if (list->strings[r].str != list->strings[i].str) {
            list->strings[++ r].str = list->strings[i].str; // copy-in next unique number
        }
    }
    list->count = r + 1;
}

void sort_string_list(struct betree_string_list* list)
{
    qsort(list->strings, list->count, sizeof(*list->strings), scmpfunc);
}

void sort_and_remove_duplicate_string_list(struct betree_string_list* list)
{
    sort_string_list(list);
    remove_duplicates_string_list(list);
}

void remove_duplicates_integer_enum_list(struct betree_integer_enum_list* list)
{
    if (list->count == 0) {
        return;
    }
    size_t r = 0;
    for (size_t i = 1; i < list->count; i++) {
        if (list->integers[r].ienum != list->integers[i].ienum) {
            list->integers[++ r] = list->integers[i]; // copy-in next unique number
        }
    }
    list->count = r + 1;
}

void sort_integer_enum_list(struct betree_integer_enum_list* list)
{
    qsort(list->integers, list->count, sizeof(*list->integers), iecmpfunc);
}

void sort_and_remove_duplicate_integer_enum_list(struct betree_integer_enum_list* list)
{
    sort_integer_enum_list(list);
    remove_duplicates_integer_enum_list(list);
}

