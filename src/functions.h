#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t betree_type_id;
typedef uint64_t betree_namespace_id;

struct frequency_type {
    const char* name;
    betree_type_id id;
};

struct frequency_namespace {
    const char* name;
    betree_namespace_id id;
};

struct frequency_cap {
    struct frequency_type type;
    uint32_t id;
    struct frequency_namespace namespace;
    bool timestamp_defined;
    int64_t timestamp;
    uint32_t value;
};

struct frequency_caps_list {
    size_t size;
    struct frequency_cap* content;
};

struct segment {
    int64_t id;
    int64_t timestamp;
};

struct segments_list {
    size_t size;
    struct segment* content;
};

bool within_frequency_caps(const struct frequency_caps_list *caps, const struct frequency_type type, uint32_t id, const struct frequency_namespace namespace, uint32_t value, size_t length, int64_t now);
bool segment_within(int64_t segment_id, int32_t after_seconds, const struct segments_list* segments, int64_t now);
bool segment_before(int64_t segment_id, int32_t before_seconds, const struct segments_list *segments, int64_t now);
bool geo_within_radius(double lat1, double lon1, double lat2, double lon2, double distance);
bool contains(const char* value, const char* pattern);
bool starts_with(const char* value, const char* pattern);
bool ends_with(const char* value, const char* pattern);
