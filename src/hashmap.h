#pragma once

#include <stddef.h>

#include "map.h"
#include "memoize.h"

struct ast_node;

typedef map_t(struct ast_node*) expr_map_t;

struct pred_map {
    betree_pred_t pred_count;
    betree_pred_t memoize_count;
    expr_map_t m;
};

void assign_pred(struct pred_map* pred_map, struct ast_node* node);
struct pred_map* make_pred_map();
void free_pred_map(struct pred_map* pred_map);

