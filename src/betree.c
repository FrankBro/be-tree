#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"

unsigned int num_of_pred_event(const struct event* event)
{
    return event->pred_count;
}

unsigned int num_of_pred_sub(const struct sub* sub) 
{
    return sub->pred_count;
}

bool match_sub(const struct event* event, const struct sub *sub)
{
    return match_node(event, sub->expr) == 1;
} 

void check_sub(const struct event* event, const struct lnode* lnode, struct matched_subs* matched_subs) 
{
    for(unsigned int i=0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        if(match_sub(event, sub) == true) {
            if(matched_subs->sub_count == 0) {
                matched_subs->subs = malloc(sizeof(int));
                if(matched_subs->subs == NULL) {
                    fprintf(stderr, "check_sub malloc failed");
                    exit(1);
                }
            }
            else {
                int* subs = realloc(matched_subs->subs, sizeof(int) * (matched_subs->sub_count + 1));
                if(sub == NULL) {
                    fprintf(stderr, "check_sub realloc failed");
                    exit(1);
                }
                matched_subs->subs = subs;
            }
            matched_subs->subs[matched_subs->sub_count] = sub->id;
            matched_subs->sub_count++;
        }
    }
}

struct pnode* search_pdir(const char* attr, const struct pdir* pdir)
{
    if(pdir == NULL) {
        return NULL;
    }
    for(unsigned int i=0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        if(strcasecmp(attr, pnode->attr) == 0) {
            return pnode;
        }
    }
    return NULL;
}

void search_cdir(const struct event* event, struct cdir* cdir, struct matched_subs* matched_subs);

void match_be_tree(const struct event* event, const struct cnode* cnode, struct matched_subs* matched_subs) 
{
    check_sub(event, cnode->lnode, matched_subs);
    for(unsigned int i = 0; i < num_of_pred_event(event); i++) {
        const char* attr = event->preds[i]->attr;
        const struct pnode* pnode = search_pdir(attr, cnode->pdir);
        if(pnode != NULL) {
            search_cdir(event, pnode->cdir, matched_subs);
        }
    }
}

bool is_event_enclosed(const struct event* event, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    for(unsigned int i = 0; i < event->pred_count; i++) {
        const char* attr = event->preds[i]->attr;
        if(strcasecmp(attr, cdir->attr) == 0) {
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
    for(unsigned int i = 0; i < sub->pred_count; i++) {
        const char* attr = sub->preds[i]->attr;
        if(strcasecmp(attr, cdir->attr) == 0) {
            struct variable_bound bound = { .min = INT32_MAX, .max = INT32_MIN };
            get_variable_bound(config->attr_domains, sub->expr, &bound);
            return cdir->startBound <= bound.min && cdir->endBound >= bound.max;
        }
    }
    return false;
}

void search_cdir(const struct event* event, struct cdir* cdir, struct matched_subs* matched_subs) 
{
    match_be_tree(event, cdir->cnode, matched_subs);
    if(is_event_enclosed(event, cdir->lChild))
        search_cdir(event, cdir->lChild, matched_subs);
    else if(is_event_enclosed(event, cdir->rChild))
        search_cdir(event, cdir->rChild, matched_subs);
}

bool is_used_cnode(const struct pred* pred, const struct cnode* cnode);

bool is_used_pdir(const struct pred* pred, const struct pdir* pdir)
{
    if(pdir == NULL || pdir->parent == NULL) {
        return false;
    }
    return is_used_cnode(pred, pdir->parent);
}

bool is_used_pnode(const struct pred* pred, const struct pnode* pnode)
{
    if(pnode == NULL || pnode->parent == NULL) {
        return false;
    }
    if(strcasecmp(pnode->attr, pred->attr) == 0) {
        return true;
    }
    return is_used_pdir(pred, pnode->parent);
}

bool is_used_cdir(const struct pred* pred, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(strcasecmp(cdir->attr, pred->attr) == 0) {
        return true;
    }
    switch(cdir->parent_type) {
        case CNODE_PARENT_PNODE: {
            return is_used_pnode(pred, cdir->pnode_parent);
        }
        case CNODE_PARENT_CDIR: {
            return is_used_cdir(pred, cdir->cdir_parent);
        }
    }
}

bool is_used_cnode(const struct pred* pred, const struct cnode* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    if(cnode->parent == NULL) {
        return false;
    }
    return is_used_cdir(pred, cnode->parent);
}

void insert_sub(const struct sub* sub, struct lnode* lnode)
{
    if(lnode->sub_count == 0) {
        lnode->subs = malloc(sizeof(struct sub*));
        if(lnode->subs == NULL) {
            fprintf(stderr, "insert_sub malloc failed");
            exit(1);
        }
    }
    else {
        struct sub** subs = realloc(lnode->subs, sizeof(struct sub*) * (lnode->sub_count + 1));
        if(sub == NULL) {
            fprintf(stderr, "insert_sub realloc failed");
            exit(1);
        }
        lnode->subs = subs;
    }
    lnode->subs[lnode->sub_count] = sub;
    lnode->sub_count++;
}

bool is_root(const struct cnode* cnode) {
    return cnode->parent == NULL; 
}

void space_partitioning(const struct config* config, struct cnode* cnode);
void space_clustering(const struct config* config, struct cdir* cdir);
struct cdir* insert_cdir(const struct config* config, const struct sub* sub, struct cdir* cdir);

unsigned int count_attr_in_lnode(const char* attr, const struct lnode* lnode);

unsigned int count_attr_in_cdir(const char* attr, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return 0;
    }
    unsigned int count = 0;
    if(cdir->cnode != NULL) {
        count += count_attr_in_lnode(attr, cdir->cnode->lnode);
    }
    count += count_attr_in_cdir(attr, cdir->lChild);
    count += count_attr_in_cdir(attr, cdir->rChild);
    return count;
}

void update_partition_score(struct pnode* pnode)
{
    // TODO: Wutdo
    float alpha = 0.5;
    unsigned int gain = count_attr_in_cdir(pnode->attr, pnode->cdir);
    // TODO: Idk man
    unsigned int loss = 0;
    pnode->score = (1.0 - alpha) * (float) gain - alpha * (float)loss;
}

void insert_be_tree(const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir)
{
    if(config == NULL) {
        fprintf(stderr, "Config is NULL, required to insert in the be tree");
        exit(1);
    }
    bool foundPartition = false;
    struct pnode* maxPnode = NULL;
    if(cnode->pdir != NULL) {
        int maxScore = -1;
        for(unsigned int i = 0; i < num_of_pred_sub(sub); i++) {
            const struct pred* pred = sub->preds[i];
            if(!is_used_cnode(pred, cnode)) {
                const char* attr = pred->attr;
                struct pnode* pnode = search_pdir(attr, cnode->pdir);
                if(pnode != NULL) {
                    foundPartition = true;
                    if(maxScore < pnode->score) {
                        maxPnode = pnode;
                        maxScore = pnode->score;
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
        struct cdir* maxCdir = insert_cdir(config, sub, maxPnode->cdir);
        insert_be_tree(config, sub, maxCdir->cnode, maxCdir);
        update_partition_score(maxPnode);
    }
}

bool is_leaf(const struct cdir* cdir)
{
    return cdir->lChild == NULL && cdir->rChild == NULL;
}

struct cdir* insert_cdir(const struct config* config, const struct sub* sub, struct cdir* cdir)
{
    if(is_leaf(cdir)) {
        return cdir;
    }
    else {
        if(sub_is_enclosed(config, sub, cdir->lChild)) {
            return insert_cdir(config, sub, cdir->lChild);
        }
        else if(sub_is_enclosed(config, sub, cdir->rChild)) {
            return insert_cdir(config, sub, cdir->rChild);
        }
        return cdir;
    }
}

bool is_overflowed(const struct lnode* lnode)
{
    return lnode->sub_count > lnode->max;
}

bool sub_has_attribute(const struct sub* sub, const char* attr)
{
    for(unsigned int i = 0; i < sub->pred_count; i++) {
        if(strcasecmp(sub->preds[i]->attr, attr) == 0) {
            return true;
        }
    }
    return false;
}

bool remove_sub(const struct sub* sub, struct lnode* lnode)
{
    for(unsigned int i = 0; i < lnode->sub_count; i++) {
        const struct sub* lnode_sub = lnode->subs[i];
        if(sub->id == lnode_sub->id) {
            for(unsigned int j = i; j < lnode->sub_count - 1; j++) {
                lnode->subs[j] = lnode->subs[j+1];
            }
            lnode->sub_count--;
            if(lnode->sub_count == 0) {
                free(lnode->subs);
                lnode->subs = NULL;
            }
            else {
                struct sub** subs = realloc(lnode->subs, sizeof(struct sub*) * lnode->sub_count);
                if(subs == NULL) {
                    fprintf(stderr, "remove_sub realloc failed");
                    exit(1);
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
        fprintf(stderr, "Could not find sub %d", sub->id);
        exit(1);
    }
    if(destination->sub_count == 0) {
        destination->subs = malloc(sizeof(struct sub*));
        if(destination->subs == NULL) {
            fprintf(stderr, "move malloc failed");
            exit(1);
        }
    }
    else {
        struct sub** subs = realloc(destination->subs, sizeof(struct sub*) * (destination->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "move realloc failed");
            exit(1);
        }
        destination->subs = subs;
    }
    destination->subs[destination->sub_count] = sub;
    destination->sub_count++;
}

struct cdir* create_cdir(const struct config* config, const char* attr, int startBound, int endBound)
{
    struct cdir* cdir = malloc(sizeof(struct cdir));
    if(cdir == NULL) {
        fprintf(stderr, "create_cdir malloc failed");
        exit(1);
    }
    cdir->attr = strdup(attr);
    if(cdir->attr == NULL) {
        fprintf(stderr, "create_cdir strdup failed");
        exit(1);
    }
    cdir->startBound = startBound;
    cdir->endBound = endBound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lChild = NULL;
    cdir->rChild = NULL;
    return cdir;
}

struct cdir* create_cdir_with_cdir_parent(const struct config* config, struct cdir* parent, int startBound, int endBound)
{
    struct cdir* cdir = create_cdir(config, parent->attr, startBound, endBound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

struct cdir* create_cdir_with_pnode_parent(const struct config* config, struct pnode* parent, int startBound, int endBound)
{
    struct cdir* cdir = create_cdir(config, parent->attr, startBound, endBound);
    cdir->parent_type = CNODE_PARENT_PNODE;
    cdir->pnode_parent = parent;
    return cdir;
}

struct pnode* create_pdir(const struct config* config, const char* attr, struct cnode* cnode)
{
    if(cnode == NULL) {
        fprintf(stderr, "cnode is NULL, cannot create a pdir and pnode");
        exit(1);
    }
    struct pdir* pdir = cnode->pdir;
    if(cnode->pdir == NULL) {
        pdir = malloc(sizeof(struct pdir));
        if(pdir == NULL) {
            fprintf(stderr, "create_pdir pdir malloc failed");
            exit(1);
        }
        pdir->parent = cnode;
        pdir->pnode_count = 0;
        pdir->pnodes = NULL;
        cnode->pdir = pdir;
    }

    struct pnode* pnode = malloc(sizeof(struct pnode));
    if(pnode == NULL) {
        fprintf(stderr, "create_pdir pnode malloc failed");
        exit(1);
    }
    pnode->parent = pdir;
    pnode->attr = strdup(attr);
    if(pnode->attr == NULL) {
        fprintf(stderr, "create_pdir strdup failed");
        exit(1);
    }
    int minBound = 0, maxBound = 0;
    bool isFound = false;
    for(unsigned int i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = &config->attr_domains[i];
        if(strcasecmp(attr_domain->name, attr) == 0) {
            minBound = attr_domain->minBound;
            maxBound = attr_domain->maxBound;
            isFound = true;
            break;
        }
    }
    if(!isFound) {
        fprintf(stderr, "No domain definition for attr %s in config", attr);
        exit(1);
    }
    pnode->cdir = create_cdir_with_pnode_parent(config, pnode, minBound, maxBound);
    pnode->score = 0;

    if(pdir->pnode_count == 0) {
        pdir->pnodes = malloc(sizeof(struct pnode));
        if(pdir->pnodes == NULL) {
            fprintf(stderr, "create_pdir pnodes malloc failed");
            exit(1);
        }
    }
    else {
        struct pnode** pnodes = realloc(pdir->pnodes, sizeof(struct pnode*) * (pdir->pnode_count + 1));
        if(pnodes == NULL) {
            fprintf(stderr, "create_pdir realloc failed");
            exit(1);
        }
        pdir->pnodes = pnodes;
    }
    pdir->pnodes[pdir->pnode_count] = pnode;
    pdir->pnode_count++;
    return pnode;
}

unsigned int count_attr_in_lnode(const char* attr, const struct lnode* lnode)
{
    int count = 0;
    if(lnode == NULL) {
        return count;
    }
    for(unsigned int i=0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        if(sub == NULL) {
            fprintf(stderr, "%s, sub is NULL", __func__);
            continue;
        }
        for(unsigned int j=0; j < sub->pred_count; j++) {
            const char* attr_to_count = sub->preds[j]->attr;
            if(attr_to_count == NULL) {
                fprintf(stderr, "%s, attr_to_count is NULL", __func__);
                continue;
            }
            if(strcasecmp(attr_to_count, attr) == 0) {
                count++;
            }
        }
    }
    return count;
}

bool is_attr_used_in_parent_cnode(const char* attr, const struct cnode* cnode);

bool is_attr_used_in_parent_pdir(const char* attr, const struct pdir* pdir)
{
    return is_attr_used_in_parent_cnode(attr, pdir->parent);
}

bool is_attr_used_in_parent_pnode(const char* attr, const struct pnode* pnode)
{
    if(strcasecmp(pnode->attr, attr) == 0) {
        return true;
    }
    return is_attr_used_in_parent_pdir(attr, pnode->parent);
}

bool is_attr_used_in_parent_cdir(const char* attr, const struct cdir* cdir)
{
    if(strcasecmp(cdir->attr, attr) == 0) {
        return true;
    }
    switch(cdir->parent_type) {
        case(CNODE_PARENT_CDIR): {
            return is_attr_used_in_parent_cdir(attr, cdir->cdir_parent);
        }
        case(CNODE_PARENT_PNODE): {
            return is_attr_used_in_parent_pnode(attr, cdir->pnode_parent);
        }
    }
}

bool is_attr_used_in_parent_cnode(const char* attr, const struct cnode* cnode)
{
    if(is_root(cnode)) {
        return false;
    }
    return is_attr_used_in_parent_cdir(attr, cnode->parent);
}

bool is_attr_used_in_parent_lnode(const char* attr, const struct lnode* lnode)
{
    return is_attr_used_in_parent_cnode(attr, lnode->parent);
}

const char* get_next_highest_score_unused_attr(const struct lnode* lnode)
{
    int highestCount = 0;
    const char* highestAttr = NULL;
    for(unsigned int i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        for(unsigned int j = 0; j < sub->pred_count; j++) {
            const char* currentAttr = sub->preds[j]->attr;
            if(!is_attr_used_in_parent_lnode(currentAttr, lnode)) {
                int currentCount = count_attr_in_lnode(currentAttr, lnode);
                if(currentCount > highestCount) {
                    highestCount = currentCount;
                    highestAttr = currentAttr;
                }
            }
        }
    }
    return highestAttr;
}

void update_cluster_capacity(const struct config* config, struct lnode* lnode)
{
    if(lnode == NULL) {
        return;
    }
    // TODO: Based on equation 9, surely wrong
    unsigned int count = lnode->sub_count;
    unsigned int max = fmax(config->lnode_max_cap, ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
    lnode->max = max;
}

void space_partitioning(const struct config* config, struct cnode* cnode) 
{
    struct lnode* lnode = cnode->lnode;
    while(is_overflowed(lnode) == true) {
        const char* attr = get_next_highest_score_unused_attr(lnode);
        if(attr == NULL) {
            break;
        }
        struct pnode* pnode = create_pdir(config, attr, cnode);
        for(unsigned int i = 0; i < lnode->sub_count; i++) {
            const struct sub* sub = lnode->subs[i];
            if(sub_has_attribute(sub, attr)) {
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
    return cdir->startBound == cdir->endBound;
}

struct lnode* make_lnode(const struct config* config, struct cnode* parent)
{
    struct lnode* lnode = malloc(sizeof(struct lnode));
    if(lnode == NULL) {
        fprintf(stderr, "make_lnode malloc failed");
        exit(1);
    }
    lnode->parent = parent;
    lnode->sub_count = 0;
    lnode->subs = NULL;
    lnode->max = config->lnode_max_cap;
    return lnode;
}

struct cnode* make_cnode(const struct config* config, struct cdir* parent)
{
    struct cnode* cnode = malloc(sizeof(struct cnode));
    if(cnode == NULL) {
        fprintf(stderr, "make_cnode malloc failed");
        exit(1);
    }
    cnode->parent = parent;
    cnode->pdir = NULL;
    cnode->lnode = make_lnode(config, cnode);
    return cnode;
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
        // TODO
        cdir->lChild = create_cdir_with_cdir_parent(config, cdir, cdir->startBound, cdir->endBound / 2);
        cdir->rChild = create_cdir_with_cdir_parent(config, cdir, cdir->endBound / 2, cdir->endBound);
        for(unsigned int i = 0; i < lnode->sub_count; i++) {
            const struct sub* sub = lnode->subs[i];
            if(sub_is_enclosed(config, sub, cdir->lChild)) {
                move(sub, lnode, cdir->lChild->cnode->lnode);
                i--;
            }
            else if(sub_is_enclosed(config, sub, cdir->rChild)) {
                move(sub, lnode, cdir->rChild->cnode->lnode);
                i--;
            }
        }
        space_partitioning(config, cdir->cnode);
        space_clustering(config, cdir->lChild);
        space_clustering(config, cdir->rChild);
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
    return cdir == NULL || (is_cnode_empty(cdir->cnode) && is_cdir_empty(cdir->lChild) && is_cdir_empty(cdir->rChild));
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
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
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
    free((char*)pred->attr);
    pred->attr = NULL;
    free(pred);
}

void free_sub(struct sub* sub)
{
    if(sub == NULL) {
        return;
    }
    for(unsigned int i = 0; i < sub->pred_count; i++) {
        const struct pred* pred = sub->preds[i];
        free_pred((struct pred*)pred);
    }
    free(sub->preds);
    sub->preds = NULL;
    free_ast_node((struct ast_node*)sub->expr);
    sub->expr = NULL;
    free(sub);
}

void free_event(struct event* event)
{
    if(event == NULL) {
        return;
    }
    for(unsigned int i = 0; i < event->pred_count; i++) {
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
    for(unsigned int i = 0; i < lnode->sub_count; i++) {
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
    free((char*)cdir->attr);
    cdir->attr = NULL;
    free_cnode(cdir->cnode);
    cdir->cnode = NULL;
    free_cdir(cdir->lChild);
    cdir->lChild = NULL;
    free_cdir(cdir->rChild);
    cdir->rChild = NULL;
    free(cdir);
}

void try_remove_pnode_from_parent(const struct pnode* pnode)
{
    struct pdir* pdir = pnode->parent;
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
        if(pnode == pdir->pnodes[i]) {
            for(unsigned int j = i; j < pdir->pnode_count - 1; j++) {
                pdir->pnodes[j] = pdir->pnodes[j+1];
            }
            pdir->pnode_count--;
            if(pdir->pnode_count == 0) {
                free(pdir->pnodes);
                pdir->pnodes = NULL;
            }
            else {
                struct pnode** pnodes = realloc(pdir->pnodes, sizeof(struct pnode*) * pdir->pnode_count);
                if(pnodes == NULL) {
                    fprintf(stderr, "try_remove_pnode_from_parent realloc failed");
                    exit(1);
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
    free((char*)pnode->attr);
    pnode->attr = NULL;
    free_cdir(pnode->cdir);
    pnode->cdir = NULL;
    free(pnode);
}

bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode) 
{
    struct pnode* pnode = NULL;
    bool isFound = delete_sub_from_leaf(sub, cnode->lnode);
    if(!isFound) {
        for(unsigned int i = 0; i < num_of_pred_sub(sub); i++) {
            const char* attr = sub->preds[i]->attr;
            pnode = search_pdir(attr, cnode->pdir);
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
            if(cdir->cdir_parent->lChild == cdir) {
                cdir->cdir_parent->lChild = NULL;
            }
            else if(cdir->cdir_parent->rChild == cdir) {
                cdir->cdir_parent->rChild = NULL;
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
    if(sub_is_enclosed(config, sub, cdir->lChild)) {
        isFound = search_delete_cdir(config, sub, cdir->lChild);
    }
    else if(sub_is_enclosed(config, sub, cdir->rChild)) {
        isFound = search_delete_cdir(config, sub, cdir->rChild);
    }
    else {
        isFound = delete_be_tree(config, sub, cdir->cnode);
    }
    if(isFound) {
        if(is_empty(cdir->lChild)) {
            remove_bucket(cdir->lChild);
            cdir->lChild = NULL;
        }
        if(is_empty(cdir->rChild)) {
            remove_bucket(cdir->rChild);
            cdir->rChild = NULL;
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
    struct matched_subs* matched_subs = malloc(sizeof(struct matched_subs));
    if(matched_subs == NULL) {
        fprintf(stderr, "make_matched_subs malloc failed");
        exit(1);
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
