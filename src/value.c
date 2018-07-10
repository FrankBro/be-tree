#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betree.h"
#include "value.h"

void add_integer_list_value(int64_t integer, struct integer_list_value* list)
{
    if(list->count == 0) {
        list->integers = calloc(1, sizeof(*list->integers));
        if(list->integers == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        int64_t* integers = realloc(list->integers, sizeof(*list->integers) * (list->count + 1));
        if(integers == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        list->integers = integers;
    }
    list->integers[list->count] = integer;
    list->count++;
}

const char* integer_list_value_to_string(struct integer_list_value list)
{
    char* string = NULL;
    for(size_t i = 0; i < list.count; i++) {
        char* new_string;
        if(i != 0) {
            if(asprintf(&new_string, "%s, %ld", string, list.integers[i]) < 0) {
                abort();
            }
            free(string);
        }
        else {
            if(asprintf(&new_string, "%ld", list.integers[i]) < 0) {
                abort();
            }
        }
        string = new_string;
    }
    return string;
}

void add_string_list_value(struct string_value string, struct string_list_value* list)
{
    if(list->count == 0) {
        list->strings = calloc(1, sizeof(*list->strings));
        if(list->strings == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct string_value* strings
            = realloc(list->strings, sizeof(*list->strings) * (list->count + 1));
        if(strings == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        list->strings = strings;
    }
    list->strings[list->count] = string;
    list->count++;
}

const char* string_list_value_to_string(struct string_list_value list)
{
    char* string = NULL;
    for(size_t i = 0; i < list.count; i++) {
        char* new_string;
        if(i != 0) {
            if(asprintf(&new_string, "%s, \"%s\"", string, list.strings[i].string) < 0) {
                abort();
            }
            free(string);
        }
        else {
            if(asprintf(&new_string, "\"%s\"", list.strings[i].string) < 0) {
                abort();
            }
        }
        string = new_string;
    }
    return string;
}

void add_segment(struct segment segment, struct segments_list* list)
{
    if(list->size == 0) {
        list->content = calloc(1, sizeof(*list->content));
        if(list->content == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct segment* content = realloc(list->content, sizeof(*list->content) * (list->size + 1));
        if(content == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        list->content = content;
    }
    list->content[list->size] = segment;
    list->size++;
}

void add_frequency(struct frequency_cap frequency, struct frequency_caps_list* list)
{
    if(list->size == 0) {
        list->content = calloc(1, sizeof(*list->content));
        if(list->content == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct frequency_cap* content
            = realloc(list->content, sizeof(*list->content) * (list->size + 1));
        if(content == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        list->content = content;
    }
    list->content[list->size] = frequency;
    list->size++;
}

struct segment make_segment(int64_t id, int64_t timestamp)
{
    struct segment segment = { .id = id, .timestamp = timestamp };
    return segment;
}

struct frequency_cap make_frequency_cap(const char* stype,
    uint32_t id,
    struct string_value namespace,
    int64_t timestamp,
    uint32_t value)
{
    enum frequency_type_e type = get_type_from_string(stype);
    struct frequency_cap frequency_cap = { .type = type,
        .id = id,
        .namespace = namespace,
        .timestamp_defined = true,
        .timestamp = timestamp,
        .value = value };
    return frequency_cap;
}

enum frequency_type_e get_type_from_string(const char* stype)
{
    if(strcmp(stype, "advertiser") == 0) {
        return FREQUENCY_TYPE_ADVERTISER;
    }
    else if(strcmp(stype, "advertiser:ip") == 0) {
        return FREQUENCY_TYPE_ADVERTISERIP;
    }
    else if(strcmp(stype, "campaign") == 0) {
        return FREQUENCY_TYPE_CAMPAIGN;
    }
    else if(strcmp(stype, "campaign:ip") == 0) {
        return FREQUENCY_TYPE_CAMPAIGNIP;
    }
    else if(strcmp(stype, "product") == 0) {
        return FREQUENCY_TYPE_PRODUCT;
    }
    else if(strcmp(stype, "product:ip") == 0) {
        return FREQUENCY_TYPE_PRODUCTIP;
    }
    else if(strcmp(stype, "flight") == 0) {
        return FREQUENCY_TYPE_FLIGHT;
    }
    else if(strcmp(stype, "flight:ip") == 0) {
        return FREQUENCY_TYPE_FLIGHTIP;
    }
    else {
        fprintf(stderr, "Invalid frequency type");
        abort();
    }
}

