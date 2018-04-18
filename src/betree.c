#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include "ast.h"
#include "betree.h"
#include "utils.h"

bool match_sub(const struct event* event, const struct sub *sub)
{
    if(sub == NULL) {
        return false;
    }
    struct value value = match_node(event, sub->expr);
    if(value.value_type != VALUE_B) {
        fprintf(stderr, "%s result is not boolean", __func__);
        abort();
    }
    return value.bvalue;
} 

void check_sub(const struct event* event, const struct lnode* lnode, struct matched_subs* matched_subs) 
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        if(match_sub(event, sub) == true) {
            if(matched_subs->sub_count == 0) {
                matched_subs->subs = calloc(1, sizeof(*matched_subs->subs));
                if(matched_subs->subs == NULL) {
                    fprintf(stderr, "%s calloc failed", __func__);
                    abort();
                }
            }
            else {
                betree_sub_t* subs = realloc(matched_subs->subs, sizeof(*matched_subs->subs) * (matched_subs->sub_count + 1));
                if(subs == NULL) {
                    fprintf(stderr, "%s realloc failed", __func__);
                    abort();
                }
                matched_subs->subs = subs;
            }
            matched_subs->subs[matched_subs->sub_count] = sub->id;
            matched_subs->sub_count++;
        }
    }
}

struct pnode* search_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    if(pdir == NULL) {
        return NULL;
    }
    for(size_t i=0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        if(variable_id == pnode->variable_id) {
            return pnode;
        }
    }
    return NULL;
}

void search_cdir(const struct config* config, const struct event* event, struct cdir* cdir, struct matched_subs* matched_subs);

bool event_contains_variable(const struct event* event, betree_var_t variable_id)
{
    for(size_t i = 0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(variable_id == pred->variable_id) {
            return true;
        }
    }
    return false;
}

void match_be_tree(const struct config* config, const struct event* event, const struct cnode* cnode, struct matched_subs* matched_subs) 
{
    check_sub(event, cnode->lnode, matched_subs);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain = get_attr_domain(config, pnode->variable_id);
            if(attr_domain == NULL) {
                const char* attr = get_attr_for_id(config, pnode->variable_id);
                fprintf(stderr, "Could not find attr_domain for attr '%s'", attr);
                abort();
            }
            if(attr_domain->allow_undefined || event_contains_variable(event, pnode->variable_id)) {
                search_cdir(config, event, pnode->cdir, matched_subs);
            }
        }
    }
}

bool is_event_enclosed(const struct event* event, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    for(size_t i = 0; i < event->pred_count; i++) {
        betree_var_t variable_id = event->preds[i]->variable_id;
        if(variable_id == cdir->variable_id) {
            return true;
        }
    }
    return false;
}

bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    for(size_t i = 0; i < sub->variable_id_count; i++) {
        betree_var_t variable_id = sub->variable_ids[i];
        if(variable_id == cdir->variable_id) {
            const struct attr_domain* attr_domain = NULL;
            for(size_t j = 0; j < config->attr_domain_count; j++) {
                const struct attr_domain* current_attr_domain = config->attr_domains[j];
                if(current_attr_domain->variable_id == variable_id) {
                    attr_domain = current_attr_domain;
                    break;
                }
            }
            if(attr_domain == NULL) {
                fprintf(stderr, "cannot find variable_id %llu in attr_domains", variable_id);
                abort();
            }
            struct value_bound bound;
            bound.value_type = attr_domain->bound.value_type;
            switch(attr_domain->bound.value_type) {
                case(VALUE_I): {
                    bound.imin = INT64_MAX;
                    bound.imax = INT64_MIN;
                    break;
                }
                case(VALUE_F): {
                    bound.fmin = DBL_MAX;
                    bound.fmax = -DBL_MAX;
                    break;
                }
                case(VALUE_B): {
                    bound.bmin = true;
                    bound.bmax = false;
                    break;
                }
                case(VALUE_S): {
                    fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
                    abort();
                }
            }
            get_variable_bound(attr_domain, sub->expr, &bound);
            switch(attr_domain->bound.value_type) {
                case(VALUE_I): {
                    return cdir->bound.imin <= bound.imin && cdir->bound.imax >= bound.imax;
                }
                case(VALUE_F): {
                    return cdir->bound.fmin <= bound.fmin && cdir->bound.fmax >= bound.fmax;
                }
                case(VALUE_B): {
                    return cdir->bound.bmin <= bound.bmin && cdir->bound.bmax >= bound.bmax;
                }
                case(VALUE_S): {
                    fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
                    abort();
                }
            }
        }
    }
    return false;
}

void search_cdir(const struct config* config, const struct event* event, struct cdir* cdir, struct matched_subs* matched_subs) 
{
    match_be_tree(config, event, cdir->cnode, matched_subs);
    if(is_event_enclosed(event, cdir->lchild))
        search_cdir(config, event, cdir->lchild, matched_subs);
    else if(is_event_enclosed(event, cdir->rchild))
        search_cdir(config, event, cdir->rchild, matched_subs);
}

bool is_used_cnode(betree_var_t variable_id, const struct cnode* cnode);

bool is_used_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    if(pdir == NULL || pdir->parent == NULL) {
        return false;
    }
    return is_used_cnode(variable_id, pdir->parent);
}

bool is_used_pnode(betree_var_t variable_id, const struct pnode* pnode)
{
    if(pnode == NULL || pnode->parent == NULL) {
        return false;
    }
    if(pnode->variable_id == variable_id) {
        return true;
    }
    return is_used_pdir(variable_id, pnode->parent);
}

bool is_used_cdir(betree_var_t variable_id, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(cdir->variable_id == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case CNODE_PARENT_PNODE: {
            return is_used_pnode(variable_id, cdir->pnode_parent);
        }
        case CNODE_PARENT_CDIR: {
            return is_used_cdir(variable_id, cdir->cdir_parent);
        }
    }
}

bool is_used_cnode(betree_var_t variable_id, const struct cnode* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    if(cnode->parent == NULL) {
        return false;
    }
    return is_used_cdir(variable_id, cnode->parent);
}

void insert_sub(const struct sub* sub, struct lnode* lnode)
{
    if(lnode->sub_count == 0) {
        lnode->subs = calloc(1, sizeof(*lnode->subs));
        if(lnode->subs == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct sub** subs = realloc(lnode->subs, sizeof(*subs) * (lnode->sub_count + 1));
        if(sub == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        lnode->subs = subs;
    }
    lnode->subs[lnode->sub_count] = (struct sub*)sub;
    lnode->sub_count++;
}

bool is_root(const struct cnode* cnode) {
    if(cnode == NULL) {
        return false;
    }
    return cnode->parent == NULL; 
}

void space_partitioning(const struct config* config, struct cnode* cnode);
void space_clustering(const struct config* config, struct cdir* cdir);
struct cdir* insert_cdir(const struct config* config, const struct sub* sub, struct cdir* cdir);

size_t count_attr_in_lnode(betree_var_t variable_id, const struct lnode* lnode);

size_t count_attr_in_cdir(betree_var_t variable_id, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return 0;
    }
    size_t count = 0;
    if(cdir->cnode != NULL) {
        count += count_attr_in_lnode(variable_id, cdir->cnode->lnode);
    }
    count += count_attr_in_cdir(variable_id, cdir->lchild);
    count += count_attr_in_cdir(variable_id, cdir->rchild);
    return count;
}

void update_partition_score(const struct config* config, struct pnode* pnode)
{
    // TODO: Wutdo
    float alpha = 0.5;
    size_t gain = count_attr_in_cdir(pnode->variable_id, pnode->cdir);
    // TODO: Idk man
    const struct attr_domain* attr_domain = get_attr_domain(config, pnode->variable_id);
    if(attr_domain == NULL) {
        const char* attr = get_attr_for_id(config, pnode->variable_id);
        fprintf(stderr, "Could not find attr_domain for attr '%s'", attr);
        abort();
    }
    uint64_t loss = attr_domain->allow_undefined ? 1.0 : 0.0;
    loss = attr_domain->bound.value_type == VALUE_S ? 2.0 : 0.0;
    pnode->score = (1.0 - alpha) * (float) gain - alpha * (float)loss;
}

void insert_be_tree(const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir)
{
    if(config == NULL) {
        fprintf(stderr, "Config is NULL, required to insert in the be tree");
        abort();
    }
    bool foundPartition = false;
    struct pnode* max_pnode = NULL;
    if(cnode->pdir != NULL) {
        float max_score = -1;
        for(size_t i = 0; i < sub->variable_id_count; i++) {
            betree_var_t variable_id = sub->variable_ids[i];
            if(!is_used_cnode(variable_id, cnode)) {
                struct pnode* pnode = search_pdir(variable_id, cnode->pdir);
                if(pnode != NULL) {
                    foundPartition = true;
                    if(max_score < pnode->score) {
                        max_pnode = pnode;
                        max_score = pnode->score;
                    }
                }
            }
        }
    }
    if(!foundPartition) {
        insert_sub(sub, cnode->lnode);
        if(is_root(cnode)) {
            space_partitioning(config, cnode);
        }
        else {
            space_clustering(config, cdir);
        }
    }
    else {
        struct cdir* maxCdir = insert_cdir(config, sub, max_pnode->cdir);
        insert_be_tree(config, sub, maxCdir->cnode, maxCdir);
        update_partition_score(config, max_pnode);
    }
}

bool is_leaf(const struct cdir* cdir)
{
    return cdir->lchild == NULL && cdir->rchild == NULL;
}

struct cdir* insert_cdir(const struct config* config, const struct sub* sub, struct cdir* cdir)
{
    if(is_leaf(cdir)) {
        return cdir;
    }
    else {
        if(sub_is_enclosed(config, sub, cdir->lchild)) {
            return insert_cdir(config, sub, cdir->lchild);
        }
        else if(sub_is_enclosed(config, sub, cdir->rchild)) {
            return insert_cdir(config, sub, cdir->rchild);
        }
        return cdir;
    }
}

bool is_overflowed(const struct lnode* lnode)
{
    return lnode->sub_count > lnode->max;
}

bool sub_has_attribute(const struct sub* sub, betree_var_t variable_id)
{
    for(size_t i = 0; i < sub->variable_id_count; i++) {
        if(sub->variable_ids[i] == variable_id) {
            return true;
        }
    }
    return false;
}

bool sub_has_attribute_str(struct config* config, const struct sub* sub, const char* attr)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return sub_has_attribute(sub, variable_id);
}

bool remove_sub(const struct sub* sub, struct lnode* lnode)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* lnode_sub = lnode->subs[i];
        if(sub->id == lnode_sub->id) {
            for(size_t j = i; j < lnode->sub_count - 1; j++) {
                lnode->subs[j] = lnode->subs[j+1];
            }
            lnode->sub_count--;
            if(lnode->sub_count == 0) {
                free(lnode->subs);
                lnode->subs = NULL;
            }
            else {
                struct sub** subs = realloc(lnode->subs, sizeof(*lnode->subs) * lnode->sub_count);
                if(subs == NULL) {
                    fprintf(stderr, "%s realloc failed", __func__);
                    abort();
                }
                lnode->subs = subs;
            }
            return true;
        }
    }
    return false;
}

void move(const struct sub* sub, struct lnode* origin, struct lnode* destination) 
{
    bool isFound = remove_sub(sub, origin);
    if(!isFound) {
        fprintf(stderr, "Could not find sub %llu", sub->id);
        abort();
    }
    if(destination->sub_count == 0) {
        destination->subs = calloc(1, sizeof(*destination->subs));
        if(destination->subs == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct sub** subs = realloc(destination->subs, sizeof(*destination->subs) * (destination->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        destination->subs = subs;
    }
    destination->subs[destination->sub_count] = (struct sub*)sub;
    destination->sub_count++;
}

struct cdir* create_cdir(const struct config* config, betree_var_t variable_id, struct value_bound bound)
{
    struct cdir* cdir = calloc(1, sizeof(*cdir));
    if(cdir == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    cdir->variable_id = variable_id;
    cdir->bound = bound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lchild = NULL;
    cdir->rchild = NULL;
    return cdir;
}

struct cdir* create_cdir_with_cdir_parent(const struct config* config, struct cdir* parent, struct value_bound bound)
{
    struct cdir* cdir = create_cdir(config, parent->variable_id, bound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

struct cdir* create_cdir_with_pnode_parent(const struct config* config, struct pnode* parent, struct value_bound bound)
{
    struct cdir* cdir = create_cdir(config, parent->variable_id, bound);
    cdir->parent_type = CNODE_PARENT_PNODE;
    cdir->pnode_parent = parent;
    return cdir;
}

struct pnode* create_pdir(const struct config* config, betree_var_t variable_id, struct cnode* cnode)
{
    if(cnode == NULL) {
        fprintf(stderr, "cnode is NULL, cannot create a pdir and pnode");
        abort();
    }
    struct pdir* pdir = cnode->pdir;
    if(cnode->pdir == NULL) {
        pdir = calloc(1, sizeof(*pdir));
        if(pdir == NULL) {
            fprintf(stderr, "%s pdir calloc failed", __func__);
            abort();
        }
        pdir->parent = cnode;
        pdir->pnode_count = 0;
        pdir->pnodes = NULL;
        cnode->pdir = pdir;
    }

    struct pnode* pnode = calloc(1, sizeof(*pnode));
    if(pnode == NULL) {
        fprintf(stderr, "%s pnode calloc failed", __func__);
        abort();
    }
    pnode->cdir = NULL;
    pnode->parent = pdir;
    pnode->variable_id = variable_id;
    pnode->score = 0.f;
    struct value_bound bound;
    bool found = false;
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->variable_id == variable_id) {
            bound = attr_domain->bound;
            found = true;
            break;
        }
    }
    if(!found) {
        fprintf(stderr, "No domain definition for attr %llu in config", variable_id);
        abort();
    }
    pnode->cdir = create_cdir_with_pnode_parent(config, pnode, bound);

    if(pdir->pnode_count == 0) {
        pdir->pnodes = calloc(1, sizeof(*pdir->pnodes));
        if(pdir->pnodes == NULL) {
            fprintf(stderr, "%s pnodes calloc failed", __func__);
            abort();
        }
    }
    else {
        struct pnode** pnodes = realloc(pdir->pnodes, sizeof(*pnodes) * (pdir->pnode_count + 1));
        if(pnodes == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        pdir->pnodes = pnodes;
    }
    pdir->pnodes[pdir->pnode_count] = pnode;
    pdir->pnode_count++;
    return pnode;
}

size_t count_attr_in_lnode(betree_var_t variable_id, const struct lnode* lnode)
{
    size_t count = 0;
    if(lnode == NULL) {
        return count;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        if(sub == NULL) {
            fprintf(stderr, "%s, sub is NULL", __func__);
            continue;
        }
        for(size_t j = 0; j < sub->variable_id_count; j++) {
            betree_var_t variable_id_to_count = sub->variable_ids[j];
            if(variable_id_to_count == variable_id) {
                count++;
            }
        }
    }
    return count;
}

bool is_attr_used_in_parent_cnode(betree_var_t variable_id, const struct cnode* cnode);

bool is_attr_used_in_parent_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    return is_attr_used_in_parent_cnode(variable_id, pdir->parent);
}

bool is_attr_used_in_parent_pnode(betree_var_t variable_id, const struct pnode* pnode)
{
    if(pnode->variable_id == variable_id) {
        return true;
    }
    return is_attr_used_in_parent_pdir(variable_id, pnode->parent);
}

bool is_attr_used_in_parent_cdir(betree_var_t variable_id, const struct cdir* cdir)
{
    if(cdir->variable_id == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case(CNODE_PARENT_CDIR): {
            return is_attr_used_in_parent_cdir(variable_id, cdir->cdir_parent);
        }
        case(CNODE_PARENT_PNODE): {
            return is_attr_used_in_parent_pnode(variable_id, cdir->pnode_parent);
        }
    }
}

bool is_attr_used_in_parent_cnode(betree_var_t variable_id, const struct cnode* cnode)
{
    if(is_root(cnode)) {
        return false;
    }
    return is_attr_used_in_parent_cdir(variable_id, cnode->parent);
}

bool is_attr_used_in_parent_lnode(betree_var_t variable_id, const struct lnode* lnode)
{
    return is_attr_used_in_parent_cnode(variable_id, lnode->parent);
}

bool get_next_highest_score_unused_attr(const struct config* config, const struct lnode* lnode, betree_var_t* variable_id)
{
    size_t highest_count = 0;
    betree_var_t highest_variable_id = 0;
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        for(size_t j = 0; j < sub->variable_id_count; j++) {
            betree_var_t current_variable_id = sub->variable_ids[j];
            const struct attr_domain* attr_domain = get_attr_domain(config, current_variable_id);
            if(attr_domain->bound.value_type != VALUE_S && !is_attr_used_in_parent_lnode(current_variable_id, lnode)) {
                size_t current_count = count_attr_in_lnode(current_variable_id, lnode);
                if(current_count > highest_count) {
                    highest_count = current_count;
                    highest_variable_id = current_variable_id;
                }
            }
        }
    }
    if(highest_count == 0) {
        return false;
    }
    else {
        *variable_id = highest_variable_id;
        return true;
    }
}

void update_cluster_capacity(const struct config* config, struct lnode* lnode)
{
    if(lnode == NULL) {
        return;
    }
    // TODO: Based on equation 9, surely wrong
    size_t count = lnode->sub_count;
    size_t max = max(config->lnode_max_cap, ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
    lnode->max = max;
}

size_t count_subs_with_variable(const struct sub** subs, size_t sub_count, betree_var_t variable_id)
{
    size_t count = 0;
    for(size_t i = 0; i < sub_count; i++) {
        const struct sub* sub = subs[i];
        if(sub_has_attribute(sub, variable_id)) {
            count++;
        }
    }
    return count;
}

void space_partitioning(const struct config* config, struct cnode* cnode) 
{
    struct lnode* lnode = cnode->lnode;
    while(is_overflowed(lnode) == true) {
        betree_var_t variable_id;
        bool found = get_next_highest_score_unused_attr(config, lnode, &variable_id);
        if(found == false) {
            break;
        }
        size_t target_subs_count = count_subs_with_variable(lnode->subs, lnode->sub_count, variable_id);
        if(target_subs_count < config->partition_min_size) {
            break;
        }
        struct pnode* pnode = create_pdir(config, variable_id, cnode);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            const struct sub* sub = lnode->subs[i];
            if(sub_has_attribute(sub, variable_id)) {
                struct cdir* cdir = insert_cdir(config, sub, pnode->cdir);
                move(sub, lnode, cdir->cnode->lnode);
                i--;
            }
        }
        space_clustering(config, pnode->cdir);
    }
    update_cluster_capacity(config, lnode);
}

bool is_atomic(const struct cdir* cdir)
{
    switch(cdir->bound.value_type) {
        case(VALUE_I): {
            return cdir->bound.imin == cdir->bound.imax;
        }
        case(VALUE_F): {
            return feq(cdir->bound.fmin, cdir->bound.fmax);
        }
        case(VALUE_B): {
            return cdir->bound.bmin == cdir->bound.bmax;
        }
        case(VALUE_S): {
            fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
            abort();
        }
    }
}

struct lnode* make_lnode(const struct config* config, struct cnode* parent)
{
    struct lnode* lnode = calloc(1, sizeof(*lnode));
    if(lnode == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    lnode->parent = parent;
    lnode->sub_count = 0;
    lnode->subs = NULL;
    lnode->max = config->lnode_max_cap;
    return lnode;
}

struct cnode* make_cnode(const struct config* config, struct cdir* parent)
{
    struct cnode* cnode = calloc(1, sizeof(*cnode));
    if(cnode == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    cnode->parent = parent;
    cnode->pdir = NULL;
    cnode->lnode = make_lnode(config, cnode);
    return cnode;
}

struct value_bounds {
    struct value_bound lbound;
    struct value_bound rbound;
};

struct value_bounds split_value_bound(struct value_bound bound)
{
    struct value_bound lbound = { .value_type = bound.value_type };
    struct value_bound rbound = { .value_type = bound.value_type };
    switch(bound.value_type) {
        case(VALUE_I): {
            int64_t start = bound.imin, end = bound.imax;
            lbound.imin = start;
            rbound.imax = end;
            if(llabs(end - start) > 2) {
                int64_t middle = start + (end - start)/2;
                lbound.imax = middle;
                rbound.imin = middle;
            }
            else if(llabs(end - start) == 2) {
                int64_t middle = start + 1;
                lbound.imax = middle;
                rbound.imin = middle;
            }
            else if(llabs(end - start) == 1) {
                lbound.imax = start;
                rbound.imin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound", __func__);
                abort();
            }
            break;
        }
        case(VALUE_F): {
            double start = bound.fmin, end = bound.fmax;
            lbound.fmin = start;
            rbound.fmax = end;
            if(fabs(end - start) > 2) {
                double middle = start + ceil((end - start)/2);
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(fabs(end - start) == 2) {
                double middle = start + 1;
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(fabs(end - start) == 1) {
                lbound.fmax = start;
                rbound.fmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound", __func__);
                abort();
            }
            break;
        }
        case(VALUE_B): {
            bool start = bound.bmin, end = bound.bmax;
            lbound.bmin = start;
            rbound.bmax = end;
            if(abs(end - start) == 1) {
                lbound.bmax = start;
                rbound.bmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound", __func__);
                abort();
            }
            break;
        }
        case(VALUE_S): {
            fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
            abort();
        }
    }
    struct value_bounds bounds = { .lbound = lbound, .rbound = rbound };
    return bounds;
}

void space_clustering(const struct config* config, struct cdir* cdir)
{
    struct lnode* lnode = cdir->cnode->lnode;
    if(!is_overflowed(lnode)) {
        return;
    }
    if(!is_leaf(cdir) || is_atomic(cdir)) {
        space_partitioning(config, cdir->cnode);
    }
    else {
        struct value_bounds bounds = split_value_bound(cdir->bound);
        cdir->lchild = create_cdir_with_cdir_parent(config, cdir, bounds.lbound);
        cdir->rchild = create_cdir_with_cdir_parent(config, cdir, bounds.rbound);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            const struct sub* sub = lnode->subs[i];
            if(sub_is_enclosed(config, sub, cdir->lchild)) {
                move(sub, lnode, cdir->lchild->cnode->lnode);
                i--;
            }
            else if(sub_is_enclosed(config, sub, cdir->rchild)) {
                move(sub, lnode, cdir->rchild->cnode->lnode);
                i--;
            }
        }
        space_partitioning(config, cdir->cnode);
        space_clustering(config, cdir->lchild);
        space_clustering(config, cdir->rchild);
    }
    update_cluster_capacity(config, lnode);
}

bool search_delete_cdir(const struct config* config, struct sub* sub, struct cdir* cdir);

bool delete_sub_from_leaf(const struct sub* sub, struct lnode* lnode)
{
    return remove_sub(sub, lnode);
}

bool is_lnode_empty(const struct lnode* lnode)
{
    return lnode == NULL || lnode->sub_count == 0;
}

bool is_pdir_empty(const struct pdir* pdir)
{
    return pdir == NULL || pdir->pnode_count == 0;
}

bool is_cnode_empty(const struct cnode* cnode)
{
    return cnode == NULL || (is_lnode_empty(cnode->lnode) && is_pdir_empty(cnode->pdir));
}

bool is_cdir_empty(const struct cdir* cdir)
{
    return cdir == NULL || (is_cnode_empty(cdir->cnode) && is_cdir_empty(cdir->lchild) && is_cdir_empty(cdir->rchild));
}

bool is_pnode_empty(const struct pnode* pnode)
{
    return pnode == NULL || (is_cdir_empty(pnode->cdir));
}

void free_pnode(struct pnode* pnode);

void free_pdir(struct pdir* pdir)
{
    if(pdir == NULL) {
        return;
    }
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        free_pnode(pnode);
    }
    free(pdir->pnodes);
    pdir->pnodes = NULL;
    free(pdir);
}

void free_pred(struct pred* pred)
{
    if(pred == NULL) {
        return;
    }
    free(pred);
}

void free_sub(struct sub* sub)
{
    if(sub == NULL) {
        return;
    }
    free(sub->variable_ids);
    sub->variable_ids = NULL;
    free_ast_node((struct ast_node*)sub->expr);
    sub->expr = NULL;
    free(sub);
}

void free_event(struct event* event)
{
    if(event == NULL) {
        return;
    }
    for(size_t i = 0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        free_pred((struct pred*)pred);
    }
    free(event->preds);
    event->preds = NULL;
    free(event);
}

void free_lnode(struct lnode* lnode)
{
    if(lnode == NULL) {
        return;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        free_sub((struct sub*)sub);
    }    
    free(lnode->subs);
    lnode->subs = NULL;
    free(lnode);
}

void free_cnode(struct cnode* cnode)
{
    if(cnode == NULL) {
        return;
    }
    free_lnode(cnode->lnode);
    cnode->lnode = NULL;
    free_pdir(cnode->pdir);
    cnode->pdir = NULL;
    free(cnode);
}

void free_cdir(struct cdir* cdir)
{
    if(cdir == NULL) {
        return;
    }
    free_cnode(cdir->cnode);
    cdir->cnode = NULL;
    free_cdir(cdir->lchild);
    cdir->lchild = NULL;
    free_cdir(cdir->rchild);
    cdir->rchild = NULL;
    free(cdir);
}

void try_remove_pnode_from_parent(const struct pnode* pnode)
{
    struct pdir* pdir = pnode->parent;
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        if(pnode == pdir->pnodes[i]) {
            for(size_t j = i; j < pdir->pnode_count - 1; j++) {
                pdir->pnodes[j] = pdir->pnodes[j+1];
            }
            pdir->pnode_count--;
            if(pdir->pnode_count == 0) {
                free(pdir->pnodes);
                pdir->pnodes = NULL;
            }
            else {
                struct pnode** pnodes = realloc(pdir->pnodes, sizeof(*pnodes) * pdir->pnode_count);
                if(pnodes == NULL) {
                    fprintf(stderr, "%s realloc failed", __func__);
                    abort();
                }
                pdir->pnodes = pnodes;
            }
            return;
        }
    }
}

void free_pnode(struct pnode* pnode)
{
    if(pnode == NULL) {
        return;
    }
    free_cdir(pnode->cdir);
    pnode->cdir = NULL;
    free(pnode);
}

bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode) 
{
    struct pnode* pnode = NULL;
    bool isFound = delete_sub_from_leaf(sub, cnode->lnode);
    if(!isFound) {
        for(size_t i = 0; i < sub->variable_id_count; i++) {
            betree_var_t variable_id = sub->variable_ids[i];
            pnode = search_pdir(variable_id, cnode->pdir);
            if(pnode != NULL) {
                isFound = search_delete_cdir(config, sub, pnode->cdir);
            }
            if(isFound) {
                break;
            }
        }
    }
    if(isFound) {
        if(pnode != NULL && is_pnode_empty(pnode)) {
            try_remove_pnode_from_parent(pnode);
            free_pnode(pnode);
        }
        if(cnode != NULL && is_pdir_empty(cnode->pdir)) {
            free_pdir(cnode->pdir);
            cnode->pdir = NULL;
        }
        if(!is_root(cnode)) {
            if(cnode != NULL && is_lnode_empty(cnode->lnode)) {
                free_lnode(cnode->lnode);
                cnode->lnode = NULL;
            }
            if(is_cnode_empty(cnode)) {
                cnode->parent->cnode = NULL;
                free_cnode(cnode);
            }
        }
    }
    return isFound;
}

bool is_empty(struct cdir* cdir)
{
    return is_cdir_empty(cdir);
}

void remove_bucket(struct cdir* cdir)
{
    free_cdir(cdir);
}

void try_remove_cdir_from_parent(struct cdir* cdir)
{
    switch(cdir->parent_type) {
        case CNODE_PARENT_CDIR: {
            if(cdir->cdir_parent->lchild == cdir) {
                cdir->cdir_parent->lchild = NULL;
            }
            else if(cdir->cdir_parent->rchild == cdir) {
                cdir->cdir_parent->rchild = NULL;
            }
            break;
        }
        case CNODE_PARENT_PNODE: {
            cdir->pnode_parent->cdir = NULL;
        }

    }
}

bool search_delete_cdir(const struct config* config, struct sub* sub, struct cdir* cdir) 
{
    bool isFound = false;
    if(sub_is_enclosed(config, sub, cdir->lchild)) {
        isFound = search_delete_cdir(config, sub, cdir->lchild);
    }
    else if(sub_is_enclosed(config, sub, cdir->rchild)) {
        isFound = search_delete_cdir(config, sub, cdir->rchild);
    }
    else {
        isFound = delete_be_tree(config, sub, cdir->cnode);
    }
    if(isFound) {
        if(is_empty(cdir->lchild)) {
            remove_bucket(cdir->lchild);
            cdir->lchild = NULL;
        }
        if(is_empty(cdir->rchild)) {
            remove_bucket(cdir->rchild);
            cdir->rchild = NULL;
        }
        if(is_empty(cdir)) {
            try_remove_cdir_from_parent(cdir);
            free_cdir(cdir);
        }
    }
    return isFound;
}

struct matched_subs* make_matched_subs()
{
    struct matched_subs* matched_subs = calloc(1, sizeof(*matched_subs));
    if(matched_subs == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    matched_subs->sub_count = 0;
    matched_subs->subs = NULL;
    return matched_subs;
}

void free_matched_subs(struct matched_subs* matched_subs)
{
    free(matched_subs->subs);
    free(matched_subs);
}

struct pred* make_simple_pred(betree_var_t variable_id, struct value value)
{
    struct pred* pred = calloc(1, sizeof(*pred));
    if(pred == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    pred->variable_id = variable_id;
    pred->value = value;
    return pred;
}

const struct pred* make_simple_pred_i(betree_var_t variable_id, int64_t ivalue)
{
    struct value value = { .value_type = VALUE_I, .ivalue = ivalue };
    return make_simple_pred(variable_id, value);
}

const struct pred* make_simple_pred_s(struct config* config, betree_var_t variable_id, const char* svalue)
{
    struct value value = { .value_type = VALUE_S, .svalue = { .string = svalue } };
    value.svalue.str = get_id_for_string(config, svalue);
    return make_simple_pred(variable_id, value);
}

const struct pred* make_simple_pred_str_i(struct config* config, const char* attr, int64_t value)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return make_simple_pred_i(variable_id, value);
}

const struct pred* make_simple_pred_str_s(struct config* config, const char* attr, const char* value)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return make_simple_pred_s(config, variable_id, value);
}

void fill_pred(struct sub* sub, const struct ast_node* expr)
{
    switch(expr->type) {
        case AST_TYPE_COMBI_EXPR: {
            fill_pred(sub, expr->combi_expr.lhs);
            fill_pred(sub, expr->combi_expr.rhs);
            return;
        }
        case AST_TYPE_BOOL_EXPR: {
            bool is_found = false;
            for(size_t i = 0; i < sub->variable_id_count; i++) {
                if(sub->variable_ids[i] == expr->bool_expr.variable_id) {
                    is_found = true;
                    break;
                }
            }
            if(!is_found) {
                if(sub->variable_id_count == 0) {
                    sub->variable_ids = calloc(1, sizeof(*sub->variable_ids));
                    if(sub->variable_ids == NULL) {
                        fprintf(stderr, "%s calloc failed", __func__);
                        abort();
                    }
                }
                else {
                    betree_var_t* variable_ids = realloc(sub->variable_ids, sizeof(*sub->variable_ids) * (sub->variable_id_count + 1));
                    if(sub == NULL) {
                        fprintf(stderr, "%s realloc failed", __func__);
                        abort();
                    }
                    sub->variable_ids = variable_ids;
                }
                sub->variable_ids[sub->variable_id_count] = expr->bool_expr.variable_id;
                sub->variable_id_count++;
            }
        }
        case AST_TYPE_BINARY_EXPR: {
            bool is_found = false;
            for(size_t i = 0; i < sub->variable_id_count; i++) {
                if(sub->variable_ids[i] == expr->binary_expr.variable_id) {
                    is_found = true;
                    break;
                }
            }
            if(!is_found) {
                if(sub->variable_id_count == 0) {
                    sub->variable_ids = calloc(1, sizeof(*sub->variable_ids));
                    if(sub->variable_ids == NULL) {
                        fprintf(stderr, "%s calloc failed", __func__);
                        abort();
                    }
                }
                else {
                    betree_var_t* variable_ids = realloc(sub->variable_ids, sizeof(*sub->variable_ids) * (sub->variable_id_count + 1));
                    if(sub == NULL) {
                        fprintf(stderr, "%s realloc failed", __func__);
                        abort();
                    }
                    sub->variable_ids = variable_ids;
                }
                sub->variable_ids[sub->variable_id_count] = expr->binary_expr.variable_id;
                sub->variable_id_count++;
            }
        }
    }
}

struct sub* make_empty_sub(betree_sub_t id)
{
    struct sub* sub = calloc(1, sizeof(*sub));
    if(sub == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    sub->id = id;
    sub->variable_id_count = 0;
    sub->variable_ids = NULL;
    return sub;
}

const struct sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr)
{
    struct sub* sub = make_empty_sub(id);
    sub->expr = expr;
    assign_variable_id(config, expr);
    fill_pred(sub, sub->expr);
    return sub;
}

const struct event* make_event()
{
    struct event* event = calloc(1, sizeof(*event));
    if(event == NULL) {
        fprintf(stderr, "%s event calloc failed", __func__);
        abort();
    }
    event->pred_count = 0;
    event->preds = NULL;
    return event;
}

const struct event* make_simple_event_i(struct config* config, const char* attr, int64_t value)
{
    struct event* event = (struct event*)make_event();
    event->pred_count = 1;
    event->preds = calloc(1, sizeof(*event->preds));
    if(event->preds == NULL) {
        fprintf(stderr, "%s preds calloc failed", __func__);
        abort();
    }
    event->preds[0] = (struct pred*)make_simple_pred_str_i(config, attr, value);
    return event;
}

const struct event* make_simple_event_s(struct config* config, const char* attr, const char* value)
{
    struct event* event = (struct event*)make_event();
    event->pred_count = 1;
    event->preds = calloc(1, sizeof(*event->preds));
    if(event->preds == NULL) {
        fprintf(stderr, "%s preds calloc failed", __func__);
        abort();
    }
    event->preds[0] = (struct pred*)make_simple_pred_str_s(config, attr, value);
    return event;
}

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id)
{
    if(variable_id < config->attr_to_id_count) {
        return config->attr_to_ids[variable_id];
    }
    return NULL;
}

betree_var_t get_id_for_attr(struct config* config, const char* attr)
{
    char* copy = strdup(attr);
    for(size_t i = 0; copy[i]; i++) {
        copy[i] = tolower(copy[i]);
    }
    for(size_t i = 0; i < config->attr_to_id_count; i++) {
        if(strcmp(config->attr_to_ids[i], copy) == 0) {
            free(copy);
            return i;
        }
    }
    if(config->attr_to_id_count == 0) {
        config->attr_to_ids = calloc(1, sizeof(*config->attr_to_ids));
        if(config->attr_to_ids == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        char** attr_to_ids = realloc(config->attr_to_ids, sizeof(*attr_to_ids) * (config->attr_to_id_count + 1));
        if(attr_to_ids == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        config->attr_to_ids = attr_to_ids;
    }
    config->attr_to_ids[config->attr_to_id_count] = copy;
    config->attr_to_id_count++;
    return config->attr_to_id_count - 1;
}

struct config* make_config(uint64_t lnode_max_cap, uint64_t partition_min_size)
{
    struct config* config = calloc(1, sizeof(*config));
    if(config == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    config->attr_domain_count = 0;
    config->attr_domains = NULL;
    config->attr_to_id_count = 0;
    config->attr_to_ids = NULL;
    config->lnode_max_cap = lnode_max_cap;
    config->partition_min_size = partition_min_size;
    config->string_value_count = 0;
    config->string_values = NULL;
    return config;
}

struct config* make_default_config()
{
    return make_config(3, 0);
}

void free_config(struct config* config)
{
    if(config == NULL) {
        return;
    }
    if(config->attr_to_ids != NULL) {
        for(size_t i = 0; i < config->attr_to_id_count; i++) {
            free(config->attr_to_ids[i]);
        }
        free(config->attr_to_ids);
        config->attr_to_ids = NULL;
    }
    if(config->attr_domains != NULL) {
        for(size_t i = 0; i < config->attr_domain_count; i++) {
            free(config->attr_domains[i]);
        }
        free(config->attr_domains);
        config->attr_domains = NULL;
    }
    if(config->string_values != NULL) {
        for(size_t i = 0; i < config->string_value_count; i++) {
            free(config->string_values[i]);
        }
        free(config->string_values);
        config->string_values = NULL;
    }
    free(config);
}

struct attr_domain* make_attr_domain(betree_var_t variable_id, struct value_bound bound, bool allow_undefined)
{
    struct attr_domain* attr_domain = calloc(1, sizeof(*attr_domain));
    if(attr_domain == NULL) {
        fprintf(stderr, "%s calloc faild", __func__);
        abort();
    }
    attr_domain->variable_id = variable_id;
    attr_domain->bound = bound;
    attr_domain->allow_undefined = allow_undefined;
    return attr_domain;
}

void add_attr_domain(struct config* config, const char* attr, struct value_bound bound, bool allow_undefined)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    struct attr_domain* attr_domain =  make_attr_domain(variable_id, bound, allow_undefined);
    if(config->attr_domain_count == 0) {
        config->attr_domains = calloc(1, sizeof(*config->attr_domains));
        if(config->attr_domains == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct attr_domain** attr_domains = realloc(config->attr_domains, sizeof(*attr_domains) * (config->attr_domain_count + 1));
        if(attr_domains == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        config->attr_domains = attr_domains;
    }
    config->attr_domains[config->attr_domain_count] = attr_domain;
    config->attr_domain_count++;
}

void add_attr_domain_i(struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_I, .imin = min, .imax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_f(struct config* config, const char* attr, double min, double max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_F, .fmin = min, .fmax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_b(struct config* config, const char* attr, bool min, bool max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_B, .bmin = min, .bmax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_S };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void adjust_attr_domains(struct config* config, const struct ast_node* node, struct value_bound bound, bool allow_undefined)
{
    switch(node->type) {
        case(AST_TYPE_BINARY_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->binary_expr.name);
            for(size_t i = 0; i < config->attr_domain_count; i++) {
                const struct attr_domain* attr_domain = config->attr_domains[i];
                if(variable_id == attr_domain->variable_id) {
                    return;
                }
            }
            add_attr_domain(config, node->binary_expr.name, bound, allow_undefined);
            break;
        }
        case(AST_TYPE_BOOL_EXPR): {
            betree_var_t variable_id = get_id_for_attr(config, node->bool_expr.name);
            for(size_t i = 0; i < config->attr_domain_count; i++) {
                const struct attr_domain* attr_domain = config->attr_domains[i];
                if(variable_id == attr_domain->variable_id) {
                    return;
                }
            }
            add_attr_domain(config, node->bool_expr.name, bound, allow_undefined);
            break;
        }
        case(AST_TYPE_COMBI_EXPR): {
            adjust_attr_domains(config, node->combi_expr.lhs, bound, allow_undefined);
            adjust_attr_domains(config, node->combi_expr.rhs, bound, allow_undefined);
            break;
        }
    }
}

void adjust_attr_domains_i(struct config* config, const struct ast_node* node, int64_t min, int64_t max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_I, .imin = min, .imax = max };
    adjust_attr_domains(config, node, bound, allow_undefined);
}

const struct attr_domain* get_attr_domain(const struct config* config, betree_var_t variable_id)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->variable_id == variable_id) {
            return attr_domain;
        }
    }
    return NULL;
}

void event_to_string(struct config* config, const struct event* event, char* buffer)
{
    size_t length = 0;
    for(size_t i = 0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(i != 0) {
            length += sprintf(buffer + length, ", ");
        }
        const char* attr = get_attr_for_id(config, pred->variable_id);
        switch(pred->value.value_type) {
            case(VALUE_I): {
                length += sprintf(buffer + length, "%s = %llu", attr, pred->value.ivalue);
                break;
            }
            case(VALUE_F): {
                length += sprintf(buffer + length, "%s = %.2f", attr, pred->value.fvalue);
                break;
            }
            case(VALUE_B): {
                const char* value = pred->value.bvalue ? "true" : "false";
                length += sprintf(buffer + length, "%s = %s", attr, value);
                break;
            }
            case(VALUE_S): {
                length += sprintf(buffer + length, "%s = \"%s\"", attr, pred->value.svalue.string);
                break;
            }
        }
    }
    buffer[length] = '\0';
}

betree_str_t get_id_for_string(struct config* config, const char* string)
{
    char* copy = strdup(string);
    for(size_t i = 0; copy[i]; i++) {
        copy[i] = tolower(copy[i]);
    }
    for(size_t i = 0; i < config->string_value_count; i++) {
        if(strcmp(config->string_values[i], copy) == 0) {
            free(copy);
            return i;
        }
    }
    if(config->string_value_count == 0) {
        config->string_values = calloc(1, sizeof(*config->string_values));
        if(config->string_values == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        char** string_values = realloc(config->string_values, sizeof(*string_values) * (config->string_value_count + 1));
        if(string_values == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        config->string_values = string_values;
    }
    config->string_values[config->string_value_count] = copy;
    config->string_value_count++;
    return config->string_value_count - 1;
}
