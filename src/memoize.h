#pragma once

#include <stdbool.h>
#include <stdint.h>

struct ast_node;

typedef uint64_t betree_pred_t;
betree_pred_t INVALID_PRED = UINT64_MAX;

struct memoize {
    uint64_t* pass;
    uint64_t* fail;
};

void set_bit(uint64_t A[], uint64_t k);
void clear_bit(uint64_t A[], uint64_t k);
bool test_bit(uint64_t A[], uint64_t k);

