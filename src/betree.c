#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "betree.h"

bool match_sub(const struct event* event, const struct sub *sub)
{
    if(sub == NULL) {
        return false;
    }
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

struct pnode* search_pdir(unsigned int variable_id, const struct pdir* pdir)
{
    if(pdir == NULL) {
        return NULL;
    }
    for(unsigned int i=0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        if(variable_id == pnode->variable_id) {
            return pnode;
        }
    }
    return NULL;
}

void search_cdir(const struct event* event, struct cdir* cdir, struct matched_subs* matched_subs);

void match_be_tree(const struct event* event, const struct cnode* cnode, struct matched_subs* matched_subs) 
{
    check_sub(event, cnode->lnode, matched_subs);
    for(unsigned int i = 0; i < event->pred_count; i++) {
        unsigned int variable_id = event->preds[i]->variable_id;
        const struct pnode* pnode = search_pdir(variable_id, cnode->pdir);
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
        unsigned int variable_id = event->preds[i]->variable_id;
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
    for(unsigned int i = 0; i < sub->variable_id_count; i++) {
        unsigned int variable_id = sub->variable_ids[i];
        if(variable_id == cdir->variable_id) {
            struct variable_bound bound = { .min = INT32_MAX, .max = INT32_MIN };
            const struct attr_domain* attr_domain = NULL;
            for(unsigned int j = 0; j < config->attr_domain_count; j++) {
                if(config->attr_domains[j]->variable_id == variable_id) {
                    attr_domain = config->attr_domains[j];
                    break;
                }
            }
            if(attr_domain == NULL) {
                fprintf(stderr, "cannot find variable_id %d in attr_domains", variable_id);
                exit(1);
            }
            else {
                get_variable_bound(attr_domain, sub->expr, &bound);
                return cdir->startBound <= bound.min && cdir->endBound >= bound.max;
            }
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

bool is_used_cnode(unsigned int variable_id, const struct cnode* cnode);

bool is_used_pdir(unsigned int variable_id, const struct pdir* pdir)
{
    if(pdir == NULL || pdir->parent == NULL) {
        return false;
    }
    return is_used_cnode(variable_id, pdir->parent);
}

bool is_used_pnode(unsigned int variable_id, const struct pnode* pnode)
{
    if(pnode == NULL || pnode->parent == NULL) {
        return false;
    }
    if(pnode->variable_id == variable_id) {
        return true;
    }
    return is_used_pdir(variable_id, pnode->parent);
}

bool is_used_cdir(unsigned int variable_id, const struct cdir* cdir)
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

bool is_used_cnode(unsigned int variable_id, const struct cnode* cnode)
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

unsigned int count_attr_in_lnode(unsigned int variable_id, const struct lnode* lnode);

unsigned int count_attr_in_cdir(unsigned int variable_id, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return 0;
    }
    unsigned int count = 0;
    if(cdir->cnode != NULL) {
        count += count_attr_in_lnode(variable_id, cdir->cnode->lnode);
    }
    count += count_attr_in_cdir(variable_id, cdir->lChild);
    count += count_attr_in_cdir(variable_id, cdir->rChild);
    return count;
}

void update_partition_score(struct pnode* pnode)
{
    // TODO: Wutdo
    float alpha = 0.5;
    unsigned int gain = count_attr_in_cdir(pnode->variable_id, pnode->cdir);
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
        for(unsigned int i = 0; i < sub->variable_id_count; i++) {
            unsigned int variable_id = sub->variable_ids[i];
            if(!is_used_cnode(variable_id, cnode)) {
                struct pnode* pnode = search_pdir(variable_id, cnode->pdir);
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

bool sub_has_attribute(const struct sub* sub, unsigned int variable_id)
{
    for(unsigned int i = 0; i < sub->variable_id_count; i++) {
        if(sub->variable_ids[i] == variable_id) {
            return true;
        }
    }
    return false;
}

bool sub_has_attribute_str(struct config* config, const struct sub* sub, const char* attr)
{
    unsigned int variable_id = get_id_for_attr(config, attr);
    return sub_has_attribute(sub, variable_id);
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
    destination->subs[destination->sub_count] = (struct sub*)sub;
    destination->sub_count++;
}

struct cdir* create_cdir(const struct config* config, unsigned int variable_id, int startBound, int endBound)
{
    struct cdir* cdir = malloc(sizeof(struct cdir));
    if(cdir == NULL) {
        fprintf(stderr, "create_cdir malloc failed");
        exit(1);
    }
    cdir->variable_id = variable_id;
    cdir->startBound = startBound;
    cdir->endBound = endBound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lChild = NULL;
    cdir->rChild = NULL;
    return cdir;
}

struct cdir* create_cdir_with_cdir_parent(const struct config* config, struct cdir* parent, int startBound, int endBound)
{
    struct cdir* cdir = create_cdir(config, parent->variable_id, startBound, endBound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

struct cdir* create_cdir_with_pnode_parent(const struct config* config, struct pnode* parent, int startBound, int endBound)
{
    struct cdir* cdir = create_cdir(config, parent->variable_id, startBound, endBound);
    cdir->parent_type = CNODE_PARENT_PNODE;
    cdir->pnode_parent = parent;
    return cdir;
}

struct pnode* create_pdir(const struct config* config, unsigned int variable_id, struct cnode* cnode)
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
    pnode->variable_id = variable_id;
    int minBound = 0, maxBound = 0;
    bool isFound = false;
    for(unsigned int i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->variable_id == variable_id) {
            minBound = attr_domain->min_bound;
            maxBound = attr_domain->max_bound;
            isFound = true;
            break;
        }
    }
    if(!isFound) {
        fprintf(stderr, "No domain definition for attr %d in config", variable_id);
        exit(1);
    }
    pnode->cdir = create_cdir_with_pnode_parent(config, pnode, minBound, maxBound);
    pnode->score = 0;

    if(pdir->pnode_count == 0) {
        pdir->pnodes = malloc(sizeof(struct pnode*));
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

unsigned int count_attr_in_lnode(unsigned int variable_id, const struct lnode* lnode)
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
        for(unsigned int j=0; j < sub->variable_id_count; j++) {
            unsigned int variable_id_to_count = sub->variable_ids[j];
            if(variable_id_to_count == variable_id) {
                count++;
            }
        }
    }
    return count;
}

bool is_attr_used_in_parent_cnode(unsigned int variable_id, const struct cnode* cnode);

bool is_attr_used_in_parent_pdir(unsigned int variable_id, const struct pdir* pdir)
{
    return is_attr_used_in_parent_cnode(variable_id, pdir->parent);
}

bool is_attr_used_in_parent_pnode(unsigned int variable_id, const struct pnode* pnode)
{
    if(pnode->variable_id == variable_id) {
        return true;
    }
    return is_attr_used_in_parent_pdir(variable_id, pnode->parent);
}

bool is_attr_used_in_parent_cdir(unsigned int variable_id, const struct cdir* cdir)
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

bool is_attr_used_in_parent_cnode(unsigned int variable_id, const struct cnode* cnode)
{
    if(is_root(cnode)) {
        return false;
    }
    return is_attr_used_in_parent_cdir(variable_id, cnode->parent);
}

bool is_attr_used_in_parent_lnode(unsigned int variable_id, const struct lnode* lnode)
{
    return is_attr_used_in_parent_cnode(variable_id, lnode->parent);
}

bool get_next_highest_score_unused_attr(const struct lnode* lnode, unsigned int* variable_id)
{
    unsigned int highest_count = 0;
    unsigned int highest_variable_id = 0;
    for(unsigned int i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        for(unsigned int j = 0; j < sub->variable_id_count; j++) {
            unsigned int current_variable_id = sub->variable_ids[j];
            if(!is_attr_used_in_parent_lnode(current_variable_id, lnode)) {
                unsigned int current_count = count_attr_in_lnode(current_variable_id, lnode);
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
    unsigned int count = lnode->sub_count;
    unsigned int max = fmax(config->lnode_max_cap, ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
    lnode->max = max;
}

void space_partitioning(const struct config* config, struct cnode* cnode) 
{
    struct lnode* lnode = cnode->lnode;
    while(is_overflowed(lnode) == true) {
        unsigned int variable_id;
        bool found = get_next_highest_score_unused_attr(lnode, &variable_id);
        if(found == false) {
            break;
        }
        struct pnode* pnode = create_pdir(config, variable_id, cnode);
        for(unsigned int i = 0; i < lnode->sub_count; i++) {
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
        int start = cdir->startBound, end = cdir->endBound;
        if(end - start > 2) {
            int middle = start + (end - start)/2;
            cdir->lChild = create_cdir_with_cdir_parent(config, cdir, start, middle);
            cdir->rChild = create_cdir_with_cdir_parent(config, cdir, middle, end);
        }
        else if(end - start == 2) {
            int middle = start + 1;
            cdir->lChild = create_cdir_with_cdir_parent(config, cdir, start, middle);
            cdir->rChild = create_cdir_with_cdir_parent(config, cdir, middle, end);
        }
        else if(end - start == 1) {
            cdir->lChild = create_cdir_with_cdir_parent(config, cdir, start, start);
            cdir->rChild = create_cdir_with_cdir_parent(config, cdir, end, end);
        }
        else {
            fprintf(stderr, "Should never happen");
            exit(1);
        }
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
    free_cdir(pnode->cdir);
    pnode->cdir = NULL;
    free(pnode);
}

bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode) 
{
    struct pnode* pnode = NULL;
    bool isFound = delete_sub_from_leaf(sub, cnode->lnode);
    if(!isFound) {
        for(unsigned int i = 0; i < sub->variable_id_count; i++) {
            unsigned int variable_id = sub->variable_ids[i];
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

const struct pred* make_simple_pred(unsigned int variable_id, int value)
{
    struct pred* pred = malloc(sizeof(struct pred));
    pred->variable_id = variable_id;
    pred->value = value;
    return pred;
}

const struct pred* make_simple_pred_str(struct config* config, const char* attr, int value)
{
    unsigned int variable_id = get_id_for_attr(config, attr);
    return make_simple_pred(variable_id, value);
}

void fill_pred(struct sub* sub, const struct ast_node* expr)
{
    switch(expr->type) {
        case AST_TYPE_COMBI_EXPR: {
            fill_pred(sub, expr->combi_expr.lhs);
            fill_pred(sub, expr->combi_expr.rhs);
            return;
        }
        case AST_TYPE_BINARY_EXPR: {
            bool is_found = false;
            for(unsigned int i = 0; i < sub->variable_id_count; i++) {
                if(sub->variable_ids[i] == expr->binary_expr.variable_id) {
                    is_found = true;
                    break;
                }
            }
            if(!is_found) {
                if(sub->variable_id_count == 0) {
                    sub->variable_ids = malloc(sizeof(int));
                }
                else {
                    unsigned int* variable_ids = realloc(sub->variable_ids, sizeof(int) * (sub->variable_id_count + 1));
                    if(sub == NULL) {
                        fprintf(stderr, "fill_pred realloc failed");
                        exit(1);
                    }
                    sub->variable_ids = variable_ids;
                }
                sub->variable_ids[sub->variable_id_count] = expr->binary_expr.variable_id;
                sub->variable_id_count++;
            }
        }
    }
}

struct sub* make_empty_sub(unsigned int id)
{
    struct sub* sub = malloc(sizeof(struct sub));
    sub->id = id;
    sub->variable_id_count = 0;
    sub->variable_ids = NULL;
    return sub;
}

const struct sub* make_sub(struct config* config, unsigned int id, struct ast_node* expr)
{
    struct sub* sub = make_empty_sub(id);
    sub->expr = expr;
    assign_variable_id(config, expr);
    fill_pred(sub, sub->expr);
    return sub;
}

const struct event* make_simple_event(struct config* config, const char* attr, int value)
{
    struct event* event = malloc(sizeof(struct event));
    event->pred_count = 1;
    event->preds = malloc(sizeof(struct pred*));
    event->preds[0] = (struct pred*)make_simple_pred_str(config, attr, value);
    return event;
}

const char* get_attr_for_id(const struct config* config, unsigned int variable_id)
{
    if(variable_id < config->attr_to_id_count) {
        return config->attr_to_ids[variable_id];
    }
    return NULL;
}

unsigned int get_id_for_attr(struct config* config, const char* attr)
{
    char* copy = strdup(attr);
    for(unsigned int i = 0; copy[i]; i++) {
        copy[i] = tolower(copy[i]);
    }
    for(unsigned int i = 0; i < config->attr_to_id_count; i++) {
        if(strcmp(config->attr_to_ids[i], copy) == 0) {
            free(copy);
            return i;
        }
    }
    if(config->attr_to_id_count == 0) {
        config->attr_to_ids = malloc(sizeof(char*));
        if(config->attr_to_ids == NULL) {
            fprintf(stderr, "get_id_for_attr malloc failed");
            exit(1);
        }
    }
    else {
        char** attr_to_ids = realloc(config->attr_to_ids, sizeof(char*) * (config->attr_to_id_count + 1));
        if(attr_to_ids == NULL) {
            fprintf(stderr, "get_id_for_attr realloc failed");
            exit(1);
        }
        config->attr_to_ids = attr_to_ids;
    }
    config->attr_to_ids[config->attr_to_id_count] = copy;
    config->attr_to_id_count++;
    return config->attr_to_id_count - 1;
}

struct config* make_config(unsigned int lnode_max_cap, unsigned int partition_min_size)
{
    struct config* config = malloc(sizeof(struct config));
    config->attr_domain_count = 0;
    config->attr_domains = NULL;
    config->attr_to_id_count = 0;
    config->attr_to_ids = NULL;
    config->lnode_max_cap = lnode_max_cap;
    config->partition_min_size = partition_min_size;
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
        for(unsigned int i = 0; i < config->attr_to_id_count; i++) {
            free(config->attr_to_ids[i]);
        }
        free(config->attr_to_ids);
        config->attr_to_ids = NULL;
    }
    if(config->attr_domains != NULL) {
        for(unsigned int i = 0; i < config->attr_domain_count; i++) {
            free(config->attr_domains[i]);
        }
        free(config->attr_domains);
        config->attr_domains = NULL;
    }
    free(config);
}

struct attr_domain* make_attr_domain(unsigned int variable_id, int min_bound, int max_bound)
{
    struct attr_domain* attr_domain = malloc(sizeof(struct attr_domain));
    attr_domain->variable_id = variable_id;
    attr_domain->min_bound = min_bound;
    attr_domain->max_bound = max_bound;
    return attr_domain;
}

void add_attr_domain(struct config* config, const char* attr, int min_bound, int max_bound)
{
    unsigned int variable_id = get_id_for_attr(config, attr);
    struct attr_domain* attr_domain =  make_attr_domain(variable_id, min_bound, max_bound);
    if(config->attr_domain_count == 0) {
        config->attr_domains = malloc(sizeof(struct attr_domain*));
        if(config->attr_domains == NULL) {
            fprintf(stderr, "add_attr_domain malloc failed");
            exit(1);
        }
    }
    else {
        struct attr_domain** attr_domains = realloc(config->attr_domains, sizeof(struct attr_domain*) * (config->attr_domain_count + 1));
        if(attr_domains == NULL) {
            fprintf(stderr, "add_attr_domain realloc failed");
            exit(1);
        }
        config->attr_domains = attr_domains;
    }
    config->attr_domains[config->attr_domain_count] = attr_domain;
    config->attr_domain_count++;
}
