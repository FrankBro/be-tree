#pragma once

#include <stddef.h>

struct ast_node;

struct pred_container {
    size_t count;
    struct ast_node** preds;
};

struct pred_compare_map {
    struct pred_container lt_preds;
    struct pred_container le_preds;
    struct pred_container gt_preds;
    struct pred_container ge_preds;
};

struct pred_equality_map {
    struct pred_container eq_preds;
    struct pred_container ne_preds;
};

struct pred_bool_map {
    struct pred_container not_preds;
    struct pred_container or_preds;
    struct pred_container and_preds;
    struct pred_container var_preds;
};

struct pred_set_map {
    struct pred_container in_preds;
    struct pred_container not_in_preds;
};

struct pred_list_map {
    struct pred_container one_of_preds;
    struct pred_container none_of_preds;
    struct pred_container all_of_preds;
};

struct pred_special_map {
    struct pred_container frequency_preds;
    struct pred_container segment_preds;
    struct pred_container geo_preds;
    struct pred_container string_preds;
};

struct pred_map {
    struct pred_compare_map compare_map;
    struct pred_equality_map equality_map;
    struct pred_bool_map bool_map;
    struct pred_set_map set_map;
    struct pred_list_map list_map;
    struct pred_special_map special_map;

    size_t pred_count; 
    struct ast_node** preds;
};

void assign_pred(struct pred_map* pred_map, struct ast_node* node);
struct pred_map* make_pred_map();
void free_pred_map(struct pred_map* pred_map);

