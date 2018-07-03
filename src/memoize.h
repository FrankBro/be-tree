#pragma once

#include <stdbool.h>
#include <stdint.h>

struct ast_node;

typedef uint64_t betree_pred_t;

struct memoize {
    uint64_t* evaluated;
    uint64_t* result;
};

void set_bit(uint64_t A[], uint64_t k);
void clear_bit(uint64_t A[], uint64_t k);
bool test_bit(uint64_t A[], uint64_t k);

