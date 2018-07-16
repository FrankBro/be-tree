#pragma once

#include <stddef.h>

#include "htable.h"

struct ast_node;

struct pred_map {
    size_t pred_count; 
    struct ast_node** preds;
    struct htable nodes;
};

void assign_pred(struct pred_map* pred_map, struct ast_node* node);
struct pred_map* make_pred_map();
void free_pred_map(struct pred_map* pred_map);

