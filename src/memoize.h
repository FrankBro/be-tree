#pragma once

#include <stdint.h>

struct ast_node;

typedef uint64_t betree_pred_t;

struct memoize {
    uint64_t* pass;
    uint64_t* fail;
};

void set_bit(uint64_t A[], uint64_t k);
void clear_bit(uint64_t A[], uint64_t k);
int test_bit(uint64_t A[], uint64_t k);

