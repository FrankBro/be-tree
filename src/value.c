#include <stdio.h>
#include <stdlib.h>

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
    char* string;
    for(size_t i = 0; i < list.count; i++) {
        if(i != 0) {
            asprintf(&string, ", ");
        }
        asprintf(&string, "%llu", list.integers[i]);
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
    char* string;
    for(size_t i = 0; i < list.count; i++) {
        if(i != 0) {
            asprintf(&string, ", ");
        }
        asprintf(&string, "\"%s\"", list.strings[i].string);
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
        int64_t* content = realloc(list->content, sizeof(*list->content) * (list->size + 1));
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
        int64_t* content = realloc(list->content, sizeof(*list->content) * (list->size + 1));
        if(content == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        list->content = content;
    }
    list->content[list->size] = frequency;
    list->size++;
}
