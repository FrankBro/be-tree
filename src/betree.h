#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "memoize.h"
#include "value.h"

typedef uint64_t betree_sub_t;

struct betree;

struct report {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    betree_sub_t* subs;
};

struct betree* betree_make();
struct betree* betree_make_with_parameters(uint64_t lnode_max_cap, uint64_t min_partition_size);
void betree_free(struct betree* betree);

void betree_add_domain(struct betree* betree, char* domain);

bool betree_insert(betree_sub_t id, const char* expr, struct betree* tree);
void betree_search(const struct betree* betree, const char* event, struct report* report);
bool betree_delete(betree_sub_t id, struct betree* betree);

struct report* make_report();
void free_report(struct report* report);

