#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tree.h"

bool within_frequency_caps(const struct betree_frequency_caps* caps,
    enum frequency_type_e type,
    uint32_t id,
    const struct string_value namespace,
    uint32_t value,
    size_t length,
    int64_t now);
bool within_frequency_caps_counting(const struct betree_frequency_caps* caps,
    enum frequency_type_e type,
    uint32_t id,
    const struct string_value namespace,
    uint32_t value,
    size_t length,
    int64_t now, int* ops_count);
bool segment_within(
    int64_t segment_id, int32_t after_seconds, const struct betree_segments* segments, int64_t now);
bool segment_within_counting(
    int64_t segment_id, int32_t after_seconds, const struct betree_segments* segments, int64_t now,
    int* ops_count);
bool segment_before(
    int64_t segment_id, int32_t before_seconds, const struct betree_segments* segments, int64_t now);
bool segment_before_counting(
    int64_t segment_id, int32_t before_seconds, const struct betree_segments* segments, int64_t now,
    int* ops_count);
bool geo_within_radius(double lat1, double lon1, double lat2, double lon2, double distance);
bool contains(const char* value, const char* pattern);
bool starts_with(const char* value, const char* pattern);
bool ends_with(const char* value, const char* pattern);
