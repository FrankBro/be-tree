#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "tree.h"
#include "utils.h"

enum short_circuit_e {
    SHORT_CIRCUIT_PASS,
    SHORT_CIRCUIT_FAIL,
    SHORT_CIRCUIT_NONE
};

enum short_circuit_e try_short_circuit(const struct short_circuit* short_circuit, const uint64_t* undefined)
{
    for(size_t i = 0; i < short_circuit->count; i++) {
        bool pass = short_circuit->pass[i] & undefined[i];
        if(pass) {
            return SHORT_CIRCUIT_PASS;
        }
        bool fail = short_circuit->fail[i] & undefined[i];
        if(fail) {
            return SHORT_CIRCUIT_FAIL;
        }
    }
    return SHORT_CIRCUIT_NONE;
}

bool match_sub(const struct config* config, const struct pred** preds, const struct sub* sub, struct report* report, struct memoize* memoize, const uint64_t* undefined)
{
    if(sub == NULL) {
        return false;
    }
    if(report != NULL) {
        report->evaluated++;
    }
    enum short_circuit_e short_circuit = try_short_circuit(&sub->short_circuit, undefined);
    if(short_circuit != SHORT_CIRCUIT_NONE) {
        if(report != NULL) {
            report->shorted++;
        }
        if(short_circuit == SHORT_CIRCUIT_PASS) {
            if(report != NULL) {
                report->matched++;
            }
            return true;
        }
        else if(short_circuit == SHORT_CIRCUIT_FAIL) {
            return false;
        }
    }
    bool result = match_node(config, preds, sub->expr, memoize, report);
    return result;
}

void check_sub(const struct config* config,
    const struct pred** preds,
    const struct lnode* lnode,
    struct report* report, 
    struct memoize* memoize,
    const uint64_t* undefined)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        if(match_sub(config, preds, sub, report, memoize, undefined) == true) {
            if(report != NULL) {
                if(report->matched == 0) {
                    report->subs = calloc(1, sizeof(*report->subs));
                    if(report->subs == NULL) {
                        fprintf(stderr, "%s calloc failed\n", __func__);
                        abort();
                    }
                }
                else {
                    betree_sub_t* subs = realloc(report->subs,
                        sizeof(*report->subs) * (report->matched + 1));
                    if(subs == NULL) {
                        fprintf(stderr, "%s realloc failed\n", __func__);
                        abort();
                    }
                    report->subs = subs;
                }
                report->subs[report->matched] = sub->id;
                report->matched++;
            }
        }
    }
}

struct pnode* search_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    if(pdir == NULL) {
        return NULL;
    }
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        if(variable_id == pnode->attr_var.var) {
            return pnode;
        }
    }
    return NULL;
}

void search_cdir(const struct config* config,
    const struct pred** preds,
    struct cdir* cdir,
    struct report* report, 
    struct memoize* memoize,
    const uint64_t* undefined);

bool event_contains_variable(const struct pred** preds, betree_var_t variable_id)
{
    return preds[variable_id] != NULL;
}

void match_be_tree(const struct config* config,
    const struct pred** preds,
    const struct cnode* cnode,
    struct report* report, 
    struct memoize* memoize,
    const uint64_t* undefined)
{
    check_sub(config, preds, cnode->lnode, report, memoize, undefined);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain = get_attr_domain(config, pnode->attr_var.var);
            if(attr_domain == NULL) {
                fprintf(stderr, "Could not find attr_domain for attr '%s'\n", pnode->attr_var.attr);
                abort();
            }
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir(config, preds, pnode->cdir, report, memoize, undefined);
            }
        }
    }
}

bool is_event_enclosed(const struct config* config, const struct pred** preds, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    const struct pred* pred = preds[cdir->attr_var.var];
    if(pred == NULL) {
        return false;
    }
    const struct attr_domain* attr_domain = get_attr_domain(config, cdir->attr_var.var);
    betree_assert(config->abort_on_error, ERROR_UNDEFINED_ATTR_DOMAIN, attr_domain != NULL);
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_TYPE_MISMATCH, attr_domain->bound.value_type == pred->value.value_type);
    switch(attr_domain->bound.value_type) {
        case VALUE_B:
            return cdir->bound.bmin <= pred->value.bvalue && cdir->bound.bmax >= pred->value.bvalue;
        case VALUE_I:
            if(pred->value.value_type == VALUE_I) {
                return cdir->bound.imin <= pred->value.ivalue && cdir->bound.imax >= pred->value.ivalue;
            }
            else if(pred->value.value_type == VALUE_IL) {
                if(pred->value.ilvalue.count != 0) {
                    return cdir->bound.imin <= pred->value.ilvalue.integers[0] && cdir->bound.imax >= pred->value.ilvalue.integers[pred->value.ilvalue.count-1];
                }
                else {
                    return true;
                }
            }
            else {
                abort();
            }
        case VALUE_F:
            return cdir->bound.fmin <= pred->value.fvalue && cdir->bound.fmax >= pred->value.fvalue;
        case VALUE_S:
            if(pred->value.value_type == VALUE_S) {
                return cdir->bound.smin <= pred->value.svalue.str && cdir->bound.smax >= pred->value.svalue.str;
            }
            else if(pred->value.value_type == VALUE_SL) {
                if(pred->value.slvalue.count != 0) {
                    return cdir->bound.smin <= pred->value.slvalue.strings[0].str && cdir->bound.smax >= pred->value.slvalue.strings[pred->value.slvalue.count-1].str;
                }
                else {
                    return true;
                }
            }
            else {
                abort();
            }
        case VALUE_IL:
            if(pred->value.value_type == VALUE_I) {
                return cdir->bound.ilmin <= pred->value.ivalue && cdir->bound.ilmax >= pred->value.ivalue;
            }
            else if(pred->value.value_type == VALUE_IL) {
                if(pred->value.ilvalue.count != 0) {
                    return cdir->bound.ilmin <= pred->value.ilvalue.integers[0] && cdir->bound.ilmax >= pred->value.ilvalue.integers[pred->value.ilvalue.count-1];
                }
                else {
                    return true;
                }
            }
            else {
                abort();
            }
        case VALUE_SL:
            if(pred->value.value_type == VALUE_S) {
                return cdir->bound.slmin <= pred->value.svalue.str && cdir->bound.slmax >= pred->value.svalue.str;
            }
            else if(pred->value.value_type == VALUE_SL) {
                if(pred->value.slvalue.count != 0) {
                    return cdir->bound.slmin <= pred->value.slvalue.strings[0].str && cdir->bound.slmax >= pred->value.slvalue.strings[pred->value.slvalue.count-1].str;
                }
                else {
                    return true;
                }
            }
            else {
                abort();
            }
        case VALUE_SEGMENTS:
        case VALUE_FREQUENCY:
            return true;
        default:
            switch_default_error("Invalid value type");
            return false;
    }
    return false;
}

bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    for(size_t i = 0; i < sub->attr_var_count; i++) {
        betree_var_t variable_id = sub->attr_vars[i].var;
        if(variable_id == cdir->attr_var.var) {
            const struct attr_domain* attr_domain = get_attr_domain(config, variable_id);
            struct value_bound bound = get_variable_bound(attr_domain, sub->expr);
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
                    return cdir->bound.smin <= bound.smin && cdir->bound.smax >= bound.smax;
                }
                case(VALUE_IL): {
                    return cdir->bound.ilmin <= bound.ilmin && cdir->bound.ilmax >= bound.ilmax;
                }
                case(VALUE_SL): {
                    return cdir->bound.slmin <= bound.slmin && cdir->bound.slmax >= bound.slmax;
                }
                case(VALUE_SEGMENTS): {
                    fprintf(
                        stderr, "%s a segments value cdir should never happen for now\n", __func__);
                    abort();
                }
                case(VALUE_FREQUENCY): {
                    fprintf(
                        stderr, "%s a frequency value cdir should never happen for now\n", __func__);
                    abort();
                }
                default: {
                    switch_default_error("Invalid bound value type");
                }
            }
        }
    }
    return false;
}

void search_cdir(const struct config* config,
    const struct pred** preds,
    struct cdir* cdir,
    struct report* report, 
    struct memoize* memoize,
    const uint64_t* undefined)
{
    match_be_tree(config, preds, cdir->cnode, report, memoize, undefined);
    if(is_event_enclosed(config, preds, cdir->lchild))
        search_cdir(config, preds, cdir->lchild, report, memoize, undefined);
    if(is_event_enclosed(config, preds, cdir->rchild))
        search_cdir(config, preds, cdir->rchild, report, memoize, undefined);
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
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_used_pdir(variable_id, pnode->parent);
}

bool is_used_cdir(betree_var_t variable_id, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(cdir->attr_var.var == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case CNODE_PARENT_PNODE: {
            return is_used_pnode(variable_id, cdir->pnode_parent);
        }
        case CNODE_PARENT_CDIR: {
            return is_used_cdir(variable_id, cdir->cdir_parent);
        }
        default: {
            switch_default_error("Invalid cdir parent type");
            return false;
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
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct sub** subs = realloc(lnode->subs, sizeof(*subs) * (lnode->sub_count + 1));
        if(sub == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        lnode->subs = subs;
    }
    lnode->subs[lnode->sub_count] = (struct sub*)sub;
    lnode->sub_count++;
}

bool is_root(const struct cnode* cnode)
{
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
    size_t gain = count_attr_in_cdir(pnode->attr_var.var, pnode->cdir);
    // TODO: Idk man
    const struct attr_domain* attr_domain = get_attr_domain(config, pnode->attr_var.var);
    if(attr_domain == NULL) {
        const char* attr = get_attr_for_id(config, pnode->attr_var.var);
        fprintf(stderr, "Could not find attr_domain for attr '%s'\n", attr);
        abort();
    }
    uint64_t loss = attr_domain->allow_undefined ? 1.0 : 0.0;
    loss += attr_domain->bound.value_type == VALUE_S && !attr_domain->bound.is_string_bounded ? 2.0 : 0.0;
    pnode->score = (1.0 - alpha) * (float)gain - alpha * (float)loss;
}

bool insert_be_tree(
    const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir)
{
    if(config == NULL) {
        fprintf(stderr, "Config is NULL, required to insert in the be tree\n");
        abort();
    }
    bool foundPartition = false;
    struct pnode* max_pnode = NULL;
    if(cnode->pdir != NULL) {
        float max_score = -1.;
        for(size_t i = 0; i < sub->attr_var_count; i++) {
            betree_var_t variable_id = sub->attr_vars[i].var;
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
    return true;
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
    for(size_t i = 0; i < sub->attr_var_count; i++) {
        if(sub->attr_vars[i].var == variable_id) {
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

bool remove_sub(betree_sub_t sub, struct lnode* lnode)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* lnode_sub = lnode->subs[i];
        if(sub == lnode_sub->id) {
            for(size_t j = i; j < lnode->sub_count - 1; j++) {
                lnode->subs[j] = lnode->subs[j + 1];
            }
            lnode->sub_count--;
            if(lnode->sub_count == 0) {
                free(lnode->subs);
                lnode->subs = NULL;
            }
            else {
                struct sub** subs = realloc(lnode->subs, sizeof(*lnode->subs) * lnode->sub_count);
                if(subs == NULL) {
                    fprintf(stderr, "%s realloc failed\n", __func__);
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
    bool isFound = remove_sub(sub->id, origin);
    if(!isFound) {
        fprintf(stderr, "Could not find sub %" PRIu64 "\n", sub->id);
        abort();
    }
    if(destination->sub_count == 0) {
        destination->subs = calloc(1, sizeof(*destination->subs));
        if(destination->subs == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct sub** subs
            = realloc(destination->subs, sizeof(*destination->subs) * (destination->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        destination->subs = subs;
    }
    destination->subs[destination->sub_count] = (struct sub*)sub;
    destination->sub_count++;
}

struct cdir* create_cdir(const struct config* config,
    const char* attr,
    betree_var_t variable_id,
    struct value_bound bound)
{
    struct cdir* cdir = calloc(1, sizeof(*cdir));
    if(cdir == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    cdir->attr_var.attr = strdup(attr);
    cdir->attr_var.var = variable_id;
    cdir->bound = bound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lchild = NULL;
    cdir->rchild = NULL;
    return cdir;
}

struct cdir* create_cdir_with_cdir_parent(
    const struct config* config, struct cdir* parent, struct value_bound bound)
{
    struct cdir* cdir = create_cdir(config, parent->attr_var.attr, parent->attr_var.var, bound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

struct cdir* create_cdir_with_pnode_parent(
    const struct config* config, struct pnode* parent, struct value_bound bound)
{
    struct cdir* cdir = create_cdir(config, parent->attr_var.attr, parent->attr_var.var, bound);
    cdir->parent_type = CNODE_PARENT_PNODE;
    cdir->pnode_parent = parent;
    return cdir;
}

struct pnode* create_pdir(
    const struct config* config, const char* attr, betree_var_t variable_id, struct cnode* cnode)
{
    if(cnode == NULL) {
        fprintf(stderr, "cnode is NULL, cannot create a pdir and pnode\n");
        abort();
    }
    struct pdir* pdir = cnode->pdir;
    if(cnode->pdir == NULL) {
        pdir = calloc(1, sizeof(*pdir));
        if(pdir == NULL) {
            fprintf(stderr, "%s pdir calloc failed\n", __func__);
            abort();
        }
        pdir->parent = cnode;
        pdir->pnode_count = 0;
        pdir->pnodes = NULL;
        cnode->pdir = pdir;
    }

    struct pnode* pnode = calloc(1, sizeof(*pnode));
    if(pnode == NULL) {
        fprintf(stderr, "%s pnode calloc failed\n", __func__);
        abort();
    }
    pnode->cdir = NULL;
    pnode->parent = pdir;
    pnode->attr_var.attr = strdup(attr);
    pnode->attr_var.var = variable_id;
    pnode->score = 0.f;
    struct value_bound bound;
    bool found = false;
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->attr_var.var == variable_id) {
            bound = attr_domain->bound;
            found = true;
            break;
        }
    }
    if(!found) {
        fprintf(stderr, "No domain definition for attr %" PRIu64 " in config\n", variable_id);
        abort();
    }
    pnode->cdir = create_cdir_with_pnode_parent(config, pnode, bound);

    if(pdir->pnode_count == 0) {
        pdir->pnodes = calloc(1, sizeof(*pdir->pnodes));
        if(pdir->pnodes == NULL) {
            fprintf(stderr, "%s pnodes calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct pnode** pnodes = realloc(pdir->pnodes, sizeof(*pnodes) * (pdir->pnode_count + 1));
        if(pnodes == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
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
            fprintf(stderr, "%s, sub is NULL\n", __func__);
            continue;
        }
        for(size_t j = 0; j < sub->attr_var_count; j++) {
            betree_var_t variable_id_to_count = sub->attr_vars[j].var;
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
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_attr_used_in_parent_pdir(variable_id, pnode->parent);
}

bool is_attr_used_in_parent_cdir(betree_var_t variable_id, const struct cdir* cdir)
{
    if(cdir->attr_var.var == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case(CNODE_PARENT_CDIR): {
            return is_attr_used_in_parent_cdir(variable_id, cdir->cdir_parent);
        }
        case(CNODE_PARENT_PNODE): {
            return is_attr_used_in_parent_pnode(variable_id, cdir->pnode_parent);
        }
        default: {
            switch_default_error("Invalid cdir parent type");
            return false;
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

bool splitable_attr_domain(const struct config* config, const struct attr_domain* attr_domain)
{
    switch(attr_domain->bound.value_type) {
        case VALUE_I:
            return ((uint64_t)llabs(attr_domain->bound.imax - attr_domain->bound.imin)) < config->max_domain_for_split;
        case VALUE_F:
            return ((uint64_t)fabs(attr_domain->bound.fmax - attr_domain->bound.fmin)) < config->max_domain_for_split;
        case VALUE_B:
            return true;
        case VALUE_S:
            return (attr_domain->bound.smax - attr_domain->bound.smin) < config->max_domain_for_split;
        case VALUE_IL:
            return ((uint64_t)llabs(attr_domain->bound.ilmax - attr_domain->bound.ilmin)) < config->max_domain_for_split;
        case VALUE_SL:
            return (attr_domain->bound.slmax - attr_domain->bound.slmin) < config->max_domain_for_split;
        case VALUE_SEGMENTS:
        case VALUE_FREQUENCY:
            return false;
        default:
            switch_default_error("Invalid bound value type");
            return false;
    }
}

bool get_next_highest_score_unused_attr(
    const struct config* config, const struct lnode* lnode, struct attr_var* attr_var)
{
    size_t highest_count = 0;
    struct attr_var highest_attr_var;
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct sub* sub = lnode->subs[i];
        for(size_t j = 0; j < sub->attr_var_count; j++) {
            struct attr_var current_attr_var = sub->attr_vars[j];
            betree_var_t current_variable_id = current_attr_var.var;
            const struct attr_domain* attr_domain = get_attr_domain(config, current_variable_id);
            if(splitable_attr_domain(config, attr_domain)
                && !is_attr_used_in_parent_lnode(current_variable_id, lnode)) {
                size_t current_count = count_attr_in_lnode(current_variable_id, lnode);
                if(current_count > highest_count) {
                    highest_count = current_count;
                    highest_attr_var = current_attr_var;
                }
            }
        }
    }
    if(highest_count == 0) {
        return false;
    }
    else {
        *attr_var = highest_attr_var;
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
    size_t max = smax(config->lnode_max_cap,
        ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
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
        struct attr_var attr_var;
        bool found = get_next_highest_score_unused_attr(config, lnode, &attr_var);
        if(found == false) {
            break;
        }
        betree_var_t variable_id = attr_var.var;
        size_t target_subs_count = count_subs_with_variable(
            (const struct sub**)lnode->subs, lnode->sub_count, variable_id);
        if(target_subs_count < config->partition_min_size) {
            break;
        }
        struct pnode* pnode = create_pdir(config, attr_var.attr, variable_id, cnode);
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
            return cdir->bound.smin == cdir->bound.smax;
        }
        case(VALUE_IL): {
            return cdir->bound.ilmin == cdir->bound.ilmax;
        }
        case(VALUE_SL): {
            return cdir->bound.slmin == cdir->bound.slmax;
        }
        case(VALUE_SEGMENTS): {
            fprintf(stderr, "%s a segments value cdir should never happen for now\n", __func__);
            abort();
        }
        case(VALUE_FREQUENCY): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: {
            switch_default_error("Invalid bound value type");
            return false;
        }
    }
}

struct lnode* make_lnode(const struct config* config, struct cnode* parent)
{
    struct lnode* lnode = calloc(1, sizeof(*lnode));
    if(lnode == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
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
        fprintf(stderr, "%s calloc failed\n", __func__);
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
                int64_t middle = start + (end - start) / 2;
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
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(VALUE_F): {
            double start = bound.fmin, end = bound.fmax;
            lbound.fmin = start;
            rbound.fmax = end;
            if(fabs(end - start) > 2) {
                double middle = start + ceil((end - start) / 2);
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(feq(fabs(end - start), 2)) {
                double middle = start + 1;
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(feq(fabs(end - start), 1)) {
                lbound.fmax = start;
                rbound.fmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
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
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(VALUE_S): {
            lbound.is_string_bounded = true;
            rbound.is_string_bounded = true;
            size_t start = bound.smin, end = bound.smax;
            lbound.smin = start;
            rbound.smax = end;
            if(end - start > 2) {
                size_t middle = start + (end - start) / 2;
                lbound.smax = middle;
                rbound.smin = middle;
            }
            else if(end - start == 2) {
                int64_t middle = start + 1;
                lbound.smax = middle;
                rbound.smin = middle;
            }
            else if(end - start == 1) {
                lbound.smax = start;
                rbound.smin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(VALUE_IL): {
            lbound.is_integer_list_bounded = true;
            rbound.is_integer_list_bounded = true;
            int64_t start = bound.ilmin, end = bound.ilmax;
            lbound.ilmin = start;
            rbound.ilmax = end;
            if(llabs(end - start) > 2) {
                int64_t middle = start + (end - start) / 2;
                lbound.ilmax = middle;
                rbound.ilmin = middle;
            }
            else if(llabs(end - start) == 2) {
                int64_t middle = start + 1;
                lbound.ilmax = middle;
                rbound.ilmin = middle;
            }
            else if(llabs(end - start) == 1) {
                lbound.ilmax = start;
                rbound.ilmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(VALUE_SL): {
            lbound.is_string_list_bounded = true;
            rbound.is_string_list_bounded = true;
            size_t start = bound.slmin, end = bound.slmax;
            lbound.slmin = start;
            rbound.slmax = end;
            if(end - start > 2) {
                size_t middle = start + (end - start) / 2;
                lbound.slmax = middle;
                rbound.slmin = middle;
            }
            else if(end - start == 2) {
                int64_t middle = start + 1;
                lbound.slmax = middle;
                rbound.slmin = middle;
            }
            else if(end - start == 1) {
                lbound.slmax = start;
                rbound.slmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(VALUE_SEGMENTS): {
            fprintf(stderr, "%s a segment value cdir should never happen for now\n", __func__);
            abort();
        }
        case(VALUE_FREQUENCY): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: {
            switch_default_error("Invalid bound value type");
        }
    }
    struct value_bounds bounds = { .lbound = lbound, .rbound = rbound };
    return bounds;
}

void space_clustering(const struct config* config, struct cdir* cdir)
{
    if(cdir == NULL || cdir->cnode == NULL) {
        return;
    }
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

bool search_delete_cdir(struct config* config, struct sub* sub, struct cdir* cdir);

bool delete_sub_from_leaf(betree_sub_t sub, struct lnode* lnode)
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
    return cdir == NULL
        || (is_cnode_empty(cdir->cnode) && is_cdir_empty(cdir->lchild)
               && is_cdir_empty(cdir->rchild));
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

void free_value(struct value value)
{
    switch(value.value_type) {
        case VALUE_IL: {
            free(value.ilvalue.integers);
            break;
        }
        case VALUE_SL: {
            for(size_t i = 0; i < value.slvalue.count; i++) {
                free((char*)value.slvalue.strings[i].string);
            }
            free(value.slvalue.strings);
            break;
        }
        case VALUE_S: {
            free((char*)value.svalue.string);
        }
        case VALUE_B:
        case VALUE_I:
        case VALUE_F: {
            break;
        }
        case VALUE_SEGMENTS: {
            free(value.segments_value.content);
            break;
        }
        case VALUE_FREQUENCY: {
            for(size_t i = 0; i < value.frequency_value.size; i++) {
                free((char*)value.frequency_value.content[i].namespace.string);
            }
            free(value.frequency_value.content);
            break;
        }
        default: {
            switch_default_error("Invalid value value type");
        }
    }
}

void free_pred(struct pred* pred)
{
    if(pred == NULL) {
        return;
    }
    free((char*)pred->attr_var.attr);
    free_value(pred->value);
    free(pred);
}

void free_sub(struct sub* sub)
{
    if(sub == NULL) {
        return;
    }
    for(size_t i = 0; i < sub->attr_var_count; i++) {
        free((char*)sub->attr_vars[i].attr);
    }
    free(sub->attr_vars);
    sub->attr_vars = NULL;
    free_ast_node((struct ast_node*)sub->expr);
    sub->expr = NULL;
    free(sub->short_circuit.pass);
    free(sub->short_circuit.fail);
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
    free((char*)cdir->attr_var.attr);
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
                pdir->pnodes[j] = pdir->pnodes[j + 1];
            }
            pdir->pnode_count--;
            if(pdir->pnode_count == 0) {
                free(pdir->pnodes);
                pdir->pnodes = NULL;
            }
            else {
                struct pnode** pnodes = realloc(pdir->pnodes, sizeof(*pnodes) * pdir->pnode_count);
                if(pnodes == NULL) {
                    fprintf(stderr, "%s realloc failed\n", __func__);
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
    free((char*)pnode->attr_var.attr);
    free_cdir(pnode->cdir);
    pnode->cdir = NULL;
    free(pnode);
}

bool betree_delete_inner(struct config* config, struct sub* sub, struct cnode* cnode)
{
    struct pnode* pnode = NULL;
    bool isFound = delete_sub_from_leaf(sub->id, cnode->lnode);
    if(!isFound) {
        for(size_t i = 0; i < sub->attr_var_count; i++) {
            betree_var_t variable_id = sub->attr_vars[i].var;
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

struct sub* find_sub_id_cdir(betree_sub_t id, struct cdir* cdir)
{
    if(cdir == NULL) {
        return NULL;
    }
    struct sub* in_cnode = find_sub_id(id, cdir->cnode);
    if(in_cnode != NULL) {
        return in_cnode;
    }
    struct sub* in_lcdir = find_sub_id_cdir(id, cdir->lchild);
    if(in_lcdir != NULL) {
        return in_lcdir;
    }
    struct sub* in_rcdir = find_sub_id_cdir(id, cdir->rchild);
    if(in_rcdir != NULL) {
        return in_rcdir;
    }
    return NULL;
}

struct sub* find_sub_id(betree_sub_t id, struct cnode* cnode)
{
    if(cnode == NULL) {
        return NULL;
    }
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        if(cnode->lnode->subs[i]->id == id) {
            return cnode->lnode->subs[i];
        }
    }
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct sub* in_cdir = find_sub_id_cdir(id, cnode->pdir->pnodes[i]->cdir);
            if(in_cdir != NULL) {
                return in_cdir;
            }
        }
    }
    return NULL;
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
            break;
        }
        default: {
            switch_default_error("Invalid cdir parent type");
        }
    }
}

bool search_delete_cdir(struct config* config, struct sub* sub, struct cdir* cdir)
{
    bool isFound = false;
    if(sub_is_enclosed(config, sub, cdir->lchild)) {
        isFound = search_delete_cdir(config, sub, cdir->lchild);
    }
    else if(sub_is_enclosed(config, sub, cdir->rchild)) {
        isFound = search_delete_cdir(config, sub, cdir->rchild);
    }
    else {
        isFound = betree_delete_inner(config, sub, cdir->cnode);
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

struct pred* make_pred(const char* attr, betree_var_t variable_id, struct value value)
{
    struct pred* pred = calloc(1, sizeof(*pred));
    if(pred == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    pred->attr_var.attr = strdup(attr);
    pred->attr_var.var = variable_id;
    pred->value = value;
    return pred;
}

void fill_pred_attr_var(struct sub* sub, struct attr_var attr_var)
{
    bool is_found = false;
    for(size_t i = 0; i < sub->attr_var_count; i++) {
        if(sub->attr_vars[i].var == attr_var.var) {
            is_found = true;
            break;
        }
    }
    if(!is_found) {
        if(sub->attr_var_count == 0) {
            sub->attr_vars = calloc(1, sizeof(*sub->attr_vars));
            if(sub->attr_vars == NULL) {
                fprintf(stderr, "%s calloc failed\n", __func__);
                abort();
            }
        }
        else {
            struct attr_var* attr_vars
                = realloc(sub->attr_vars, sizeof(*sub->attr_vars) * (sub->attr_var_count + 1));
            if(sub == NULL) {
                fprintf(stderr, "%s realloc failed\n", __func__);
                abort();
            }
            sub->attr_vars = attr_vars;
        }
        sub->attr_vars[sub->attr_var_count] = copy_attr_var(attr_var);
        sub->attr_var_count++;
    }
}

void fill_pred(struct sub* sub, const struct ast_node* expr)
{
    switch(expr->type) {
        case AST_TYPE_SPECIAL_EXPR: {
            switch(expr->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    fill_pred_attr_var(sub, expr->special_expr.frequency.attr_var);
                    return;
                case AST_SPECIAL_GEO:
                    fill_pred_attr_var(sub, expr->special_expr.geo.latitude_var);
                    fill_pred_attr_var(sub, expr->special_expr.geo.longitude_var);
                    return;
                case AST_SPECIAL_STRING:
                    fill_pred_attr_var(sub, expr->special_expr.string.attr_var);
                    return;
                case AST_SPECIAL_SEGMENT:
                    fill_pred_attr_var(sub, expr->special_expr.segment.attr_var);
                    return;
                default:
                    switch_default_error("Invalid special expr type");
                    return;
            }
            return;
        }
        case AST_TYPE_BOOL_EXPR: {
            switch(expr->bool_expr.op) {
                case AST_BOOL_AND:
                case AST_BOOL_OR:
                    fill_pred(sub, expr->bool_expr.binary.lhs);
                    fill_pred(sub, expr->bool_expr.binary.rhs);
                    return;
                case AST_BOOL_NOT:
                    fill_pred(sub, expr->bool_expr.unary.expr);
                    return;
                case AST_BOOL_VARIABLE:
                    fill_pred_attr_var(sub, expr->bool_expr.variable);
                    return;
                default:
                    switch_default_error("Invalid bool op");
                    return;
            }
            return;
        }
        case AST_TYPE_NUMERIC_COMPARE_EXPR: {
            fill_pred_attr_var(sub, expr->numeric_compare_expr.attr_var);
            return;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            fill_pred_attr_var(sub, expr->equality_expr.attr_var);
            return;
        }
        case AST_TYPE_SET_EXPR: {
            if(expr->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                fill_pred_attr_var(sub, expr->set_expr.left_value.variable_value);
            }
            else if(expr->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
                fill_pred_attr_var(sub, expr->set_expr.right_value.variable_value);
            }
            else {
                return;
            }
            return;
        }
        case AST_TYPE_LIST_EXPR: {
            fill_pred_attr_var(sub, expr->list_expr.attr_var);
            return;
        }
        default: {
            switch_default_error("Invalid expr type");
        }
    }
}

struct sub* make_empty_sub(betree_sub_t id)
{
    struct sub* sub = calloc(1, sizeof(*sub));
    if(sub == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    sub->id = id;
    sub->attr_var_count = 0;
    sub->attr_vars = NULL;
    return sub;
}

enum short_circuit_e short_circuit_for_attr_var(betree_var_t id, bool inverted, struct attr_var attr_var) {
    if(id == attr_var.var) {
        if(inverted) {
            return SHORT_CIRCUIT_PASS;
        }
        else {
            return SHORT_CIRCUIT_FAIL;
        }
    }
    else {
        return SHORT_CIRCUIT_NONE;
    }
}

enum short_circuit_e short_circuit_for_node(betree_var_t id, bool inverted, const struct ast_node* node) {
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR: 
            return short_circuit_for_attr_var(id, inverted, node->numeric_compare_expr.attr_var);
        case AST_TYPE_EQUALITY_EXPR: 
            return short_circuit_for_attr_var(id, inverted, node->equality_expr.attr_var);
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR: {
                    enum short_circuit_e lhs = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
                    if(lhs == SHORT_CIRCUIT_PASS || rhs == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    if(lhs == SHORT_CIRCUIT_FAIL && rhs == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_BOOL_AND: {
                    enum short_circuit_e lhs = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
                    if(lhs == SHORT_CIRCUIT_FAIL || rhs == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(lhs == SHORT_CIRCUIT_PASS && rhs == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_BOOL_NOT:
                    return short_circuit_for_node(id, !inverted, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return short_circuit_for_attr_var(id, inverted, node->bool_expr.variable);
                default:
                    abort();
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                return short_circuit_for_attr_var(id, inverted, node->set_expr.left_value.variable_value);
            }
            else {
                return short_circuit_for_attr_var(id, inverted, node->set_expr.right_value.variable_value);
            }
        case AST_TYPE_LIST_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->list_expr.attr_var);
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    return short_circuit_for_attr_var(id, inverted, node->special_expr.frequency.attr_var);
                case AST_SPECIAL_SEGMENT:
                    return short_circuit_for_attr_var(id, inverted, node->special_expr.segment.attr_var);
                case AST_SPECIAL_GEO:
                    return short_circuit_for_attr_var(id, inverted, node->special_expr.geo.latitude_var) || short_circuit_for_attr_var(id, inverted, node->special_expr.geo.longitude_var);
                case AST_SPECIAL_STRING:
                    return short_circuit_for_attr_var(id, inverted, node->special_expr.string.attr_var);
                default:
                    abort();
            }
        default:
            abort();
    }
    return SHORT_CIRCUIT_NONE;
}

void fill_short_circuit(struct config* config, struct sub* sub)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->allow_undefined) {
            enum short_circuit_e result = short_circuit_for_node(attr_domain->attr_var.var, false, sub->expr);
            if(result == SHORT_CIRCUIT_PASS) {
                set_bit(sub->short_circuit.pass, i);
            }
            else if(result == SHORT_CIRCUIT_FAIL) {
                set_bit(sub->short_circuit.fail, i);
            }
        }
    }
}

struct sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr)
{
    struct sub* sub = make_empty_sub(id);
    sub->expr = expr;
    assign_variable_id(config, expr);
    fill_pred(sub, sub->expr);
    size_t count = config->attr_domain_count / 64 + 1;
    sub->short_circuit.count = count;
    sub->short_circuit.pass = calloc(count, sizeof(*sub->short_circuit.pass));
    sub->short_circuit.fail = calloc(count, sizeof(*sub->short_circuit.fail));
    fill_short_circuit(config, sub);
    return sub;
}

struct event* make_event()
{
    struct event* event = calloc(1, sizeof(*event));
    if(event == NULL) {
        fprintf(stderr, "%s event calloc failed\n", __func__);
        abort();
    }
    event->pred_count = 0;
    event->preds = NULL;
    return event;
}

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id)
{
    if(variable_id < config->attr_to_id_count) {
        return config->attr_to_ids[variable_id];
    }
    return NULL;
}

betree_var_t try_get_id_for_attr(const struct config* config, const char* attr)
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
    free(copy);
    return UINT64_MAX;
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
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        char** attr_to_ids
            = realloc(config->attr_to_ids, sizeof(*attr_to_ids) * (config->attr_to_id_count + 1));
        if(attr_to_ids == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
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
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    config->abort_on_error = true;
    config->attr_domain_count = 0;
    config->attr_domains = NULL;
    config->attr_to_id_count = 0;
    config->attr_to_ids = NULL;
    config->lnode_max_cap = lnode_max_cap;
    config->partition_min_size = partition_min_size;
    config->max_domain_for_split = 1000;
    config->string_map_count = 0;
    config->string_maps = NULL;
    config->pred_map = make_pred_map();
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
            free((char*)config->attr_domains[i]->attr_var.attr);
            free(config->attr_domains[i]);
        }
        free(config->attr_domains);
        config->attr_domains = NULL;
    }
    if(config->string_maps != NULL) {
        for(size_t i = 0; i < config->string_map_count; i++) {
            free((char*)config->string_maps[i].attr_var.attr);
            for(size_t j = 0; j < config->string_maps[i].string_value_count; j++) {
                free(config->string_maps[i].string_values[j]);
            }
            free(config->string_maps[i].string_values);
        }
        free(config->string_maps);
        config->string_maps = NULL;
    }
    if(config->pred_map != NULL) {
        free_pred_map(config->pred_map);
        config->pred_map = NULL;
    }
    free(config);
}

struct attr_domain* make_attr_domain(
    const char* attr, betree_var_t variable_id, struct value_bound bound, bool allow_undefined)
{
    struct attr_domain* attr_domain = calloc(1, sizeof(*attr_domain));
    if(attr_domain == NULL) {
        fprintf(stderr, "%s calloc faild\n", __func__);
        abort();
    }
    attr_domain->attr_var.attr = strdup(attr);
    attr_domain->attr_var.var = variable_id;
    attr_domain->bound = bound;
    attr_domain->allow_undefined = allow_undefined;
    return attr_domain;
}

void add_attr_domain(
    struct config* config, const char* attr, struct value_bound bound, bool allow_undefined)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    struct attr_domain* attr_domain = make_attr_domain(attr, variable_id, bound, allow_undefined);
    if(config->attr_domain_count == 0) {
        config->attr_domains = calloc(1, sizeof(*config->attr_domains));
        if(config->attr_domains == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct attr_domain** attr_domains = realloc(
            config->attr_domains, sizeof(*attr_domains) * (config->attr_domain_count + 1));
        if(attr_domains == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        config->attr_domains = attr_domains;
    }
    config->attr_domains[config->attr_domain_count] = attr_domain;
    config->attr_domain_count++;
}

void add_attr_domain_i(
    struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_I, .imin = min, .imax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_f(
    struct config* config, const char* attr, double min, double max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_F, .fmin = min, .fmax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_b(
    struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_B, .bmin = false, .bmax = true };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_S };
    bound.is_string_bounded = false;
    bound.smin = 0;
    bound.smax = (size_t)-1;
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_bounded_s(struct config* config, const char* attr, bool allow_undefined, size_t max)
{
    struct value_bound bound = { .value_type = VALUE_S };
    bound.is_string_bounded = true;
    bound.smin = 0;
    bound.smax = max - 1;
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_IL };
    bound.ilmin = INT64_MIN;
    bound.ilmax = INT64_MAX;
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_bounded_il(struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_IL };
    bound.is_integer_list_bounded = true;
    bound.ilmin = min;
    bound.ilmax = max;
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_SL };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_bounded_sl(struct config* config, const char* attr, bool allow_undefined, size_t max)
{
    struct value_bound bound = { .value_type = VALUE_SL };
    bound.is_string_list_bounded = true;
    bound.slmin = 0;
    bound.slmax = max - 1;
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_SEGMENTS };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_FREQUENCY };
    add_attr_domain(config, attr, bound, allow_undefined);
}

const struct attr_domain* get_attr_domain(const struct config* config, betree_var_t variable_id)
{
    return config->attr_domains[variable_id];
}

void event_to_string(const struct event* event, char* buffer)
{
    size_t length = 0;
    for(size_t i = 0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(i != 0) {
            length += sprintf(buffer + length, ", ");
        }
        const char* attr = pred->attr_var.attr;
        switch(pred->value.value_type) {
            case(VALUE_I): {
                length += sprintf(buffer + length, "%s = %" PRIu64, attr, pred->value.ivalue);
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
            case(VALUE_IL): {
                const char* integer_list = integer_list_value_to_string(pred->value.ilvalue);
                length += sprintf(buffer + length, "%s = (%s)", attr, integer_list);
                free((char*)integer_list);
                break;
            }
            case(VALUE_SEGMENTS):
            case(VALUE_FREQUENCY): {
                fprintf(stderr, "TODO\n");
                abort();
                break;
            }
            case(VALUE_SL): {
                const char* string_list = string_list_value_to_string(pred->value.slvalue);
                length += sprintf(buffer + length, "%s = (%s)", attr, string_list);
                free((char*)string_list);
                break;
            }
            default: {
                switch_default_error("Invalid value value type");
            }
        }
    }
    buffer[length] = '\0';
}

void add_string_map(struct attr_var attr_var, struct config* config)
{
    if(config->string_map_count == 0) {
        config->string_maps = calloc(1, sizeof(*config->string_maps));
        if(config->string_maps == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct string_map* string_maps = realloc(
            config->string_maps, sizeof(*string_maps) * (config->string_map_count + 1));
        if(string_maps == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        config->string_maps = string_maps;
    }
    config->string_maps[config->string_map_count].attr_var.attr = strdup(attr_var.attr);
    config->string_maps[config->string_map_count].attr_var.var = attr_var.var;
    config->string_maps[config->string_map_count].string_value_count = 0;
    config->string_maps[config->string_map_count].string_values = 0;
    config->string_map_count++;
}

void add_to_string_map(struct string_map* string_map, char* copy)
{
    if(string_map->string_value_count == 0) {
        string_map->string_values = calloc(1, sizeof(*string_map->string_values));
        if(string_map->string_values == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        char** string_values = realloc(
            string_map->string_values, sizeof(*string_values) * (string_map->string_value_count + 1));
        if(string_values == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        string_map->string_values = string_values;
    }
    string_map->string_values[string_map->string_value_count] = copy;
    string_map->string_value_count++;
}

betree_str_t try_get_id_for_string(const struct config* config, struct attr_var attr_var, const char* string)
{
    char* copy = strdup(string);
    for(size_t i = 0; i < config->string_map_count; i++) {
        if(config->string_maps[i].attr_var.var == attr_var.var) {
            struct string_map* string_map = string_map = &config->string_maps[i];
            for(size_t j = 0; j < string_map->string_value_count; j++) {
                if(strcmp(string_map->string_values[j], copy) == 0) {
                    free(copy);
                    return j;
                }
            }
            break;
        }
    }
    free(copy);
    return UINT64_MAX;
}

betree_str_t get_id_for_string(struct config* config, struct attr_var attr_var, const char* string)
{
    char* copy = strdup(string);
    struct string_map* string_map = NULL;
    for(size_t i = 0; i < config->string_map_count; i++) {
        if(config->string_maps[i].attr_var.var == attr_var.var) {
            string_map = &config->string_maps[i];
            for(size_t j = 0; j < string_map->string_value_count; j++) {
                if(strcmp(string_map->string_values[j], copy) == 0) {
                    free(copy);
                    return j;
                }
            }
            break;
        }
    }
    if(string_map == NULL) {
        add_string_map(attr_var, config);
        string_map = &config->string_maps[config->string_map_count - 1];
    }
    const struct attr_domain* attr_domain = get_attr_domain(config, attr_var.var);
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_TYPE_MISMATCH, attr_domain != NULL && 
        (attr_domain->bound.value_type == VALUE_S || attr_domain->bound.value_type == VALUE_SL || attr_domain->bound.value_type == VALUE_FREQUENCY));
    if(attr_domain->bound.is_string_bounded && attr_domain->bound.smax + 1 == string_map->string_value_count) {
        free(copy);
        return UINT64_MAX;
    }
    add_to_string_map(string_map, copy);
    return string_map->string_value_count - 1;
}

bool is_variable_allow_undefined(const struct config* config, const betree_var_t variable_id)
{
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_MISSING, variable_id < config->attr_domain_count);
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_NOT_INDEX, config->attr_domains[variable_id]->attr_var.var == variable_id);
    return config->attr_domains[variable_id]->allow_undefined;
}

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct event** event);

bool betree_can_insert(const struct config* config, betree_sub_t id, const char* expr, struct cnode* cnode)
{
    (void)id;
    (void)cnode;
    struct ast_node* node;
    if(parse(expr, &node)) {
        return false;
    }
    bool var = all_variables_in_config(config, node);
    bool str = all_bounded_strings_valid(config, node);
    return var && str;
}

struct memoize make_memoize(size_t pred_count)
{
    size_t count = pred_count / 64 + 1;
    struct memoize memoize = {
        .pass = calloc(count, sizeof(*memoize.pass)),
        .fail = calloc(count, sizeof(*memoize.fail)),
    };
    return memoize;
}

void free_memoize(struct memoize memoize)
{
    free(memoize.pass);
    free(memoize.fail);
}

void init_undefined(const struct config* config, const struct event* event, uint64_t* undefined)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->allow_undefined) {
            bool found = false;
            for(size_t j = 0; j < event->pred_count; j++) {
                const struct pred* pred = event->preds[j];
                if(attr_domain->attr_var.var == pred->attr_var.var) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                set_bit(undefined, i);
            }
        }
    }
}

struct pred** make_environment(const struct config* config, const struct event* event)
{
    struct pred** preds = calloc(config->attr_domain_count, sizeof(*preds));
    for(size_t i = 0; i < event->pred_count; i++) {
        preds[event->preds[i]->attr_var.var] = event->preds[i];
    }
    return preds;
}

void betree_search_with_event(const struct config* config,
    struct event* event,
    const struct cnode* cnode,
    struct report* report)
{
    uint64_t* undefined = calloc(config->attr_domain_count, sizeof(*undefined));
    init_undefined(config, event, undefined);
    struct memoize memoize = make_memoize(config->pred_map->pred_count);
    struct pred** preds = make_environment(config, event);
    match_be_tree(config, (const struct pred**)preds, cnode, report, &memoize, undefined);
    free_memoize(memoize);
    free(undefined);
    free_event(event);
    free(preds);
}

void sort_event_lists(struct event* event)
{
    for(size_t i = 0; i < event->pred_count; i++) {
        struct pred* pred = event->preds[i];
        if(pred->value.value_type == VALUE_IL) {
            qsort(pred->value.ilvalue.integers,
              pred->value.ilvalue.count,
              sizeof(*pred->value.ilvalue.integers),
              icmpfunc);
        }
        else if(pred->value.value_type == VALUE_SL) {
            qsort(pred->value.slvalue.strings,
              pred->value.slvalue.count,
              sizeof(*pred->value.slvalue.strings),
              scmpfunc);
        }
    }
}

struct event* make_event_from_string(const struct config* config, const char* event_str)
{
    struct event* event;
    if(event_parse(event_str, &event)) {
        fprintf(stderr, "Failed to parse event: %s\n", event_str);
        abort();
    }
    fill_event(config, event);
    if(validate_event(config, event) == false) {
        fprintf(stderr, "Failed to validate event: %s\n", event_str);
        abort();
    }
    sort_event_lists(event);
    return event;
}

struct attr_var make_attr_var(const char* attr, struct config* config)
{
    struct attr_var attr_var;
    attr_var.attr = attr == NULL ? NULL : strdup(attr);
    if(config == NULL) {
        attr_var.var = -1ULL;
    }
    else {
        attr_var.var = get_id_for_attr(config, attr);
    }
    return attr_var;
}

void free_attr_var(struct attr_var attr_var)
{
    free((char*)attr_var.attr);
}

struct attr_var copy_attr_var(struct attr_var attr_var)
{
    struct attr_var copy = { .attr = strdup(attr_var.attr), .var = attr_var.var };
    return copy;
}

void add_pred(struct pred* pred, struct event* event)
{
    if(pred == NULL) {
        return;
    }
    if(event->pred_count == 0) {
        event->preds = calloc(1, sizeof(*event->preds));
        if(event->preds == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct pred** preds = realloc(event->preds, sizeof(*preds) * (event->pred_count + 1));
        if(preds == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        event->preds = preds;
    }
    event->preds[event->pred_count] = pred;
    event->pred_count++;
}

void fill_event(const struct config* config, struct event* event)
{
    for(size_t i = 0; i < event->pred_count; i++) {
        struct pred* pred = event->preds[i];
        if(pred->attr_var.var == -1ULL) {
            betree_var_t var = try_get_id_for_attr(config, pred->attr_var.attr);
            if(var == UINT64_MAX) {
                fprintf(stderr, "Cannot find variable %s in config, aborting", pred->attr_var.attr);
                abort();
            }
            pred->attr_var.var = var;
        }
        switch(pred->value.value_type) {
            case VALUE_B:
            case VALUE_I:
            case VALUE_F:
            case VALUE_IL:
            case VALUE_SEGMENTS:
                break;
            case VALUE_S: {
                if(pred->value.svalue.str == -1ULL) {
                    betree_str_t str = try_get_id_for_string(config, pred->attr_var, pred->value.svalue.string);
                    pred->value.svalue.var = pred->attr_var.var;
                    pred->value.svalue.str = str;
                }
                break;
            }
            case VALUE_SL: {
                for(size_t j = 0; j < pred->value.slvalue.count; j++) {
                    if(pred->value.slvalue.strings[j].str == -1ULL) {
                        betree_str_t str
                            = try_get_id_for_string(config, pred->attr_var, pred->value.slvalue.strings[j].string);
                        pred->value.slvalue.strings[j].var = pred->attr_var.var;
                        pred->value.slvalue.strings[j].str = str;
                    }
                }
                break;
            }
            case VALUE_FREQUENCY: {
                for(size_t j = 0; j < pred->value.frequency_value.size; j++) {
                    if(pred->value.frequency_value.content[j].namespace.str == -1ULL) {
                        betree_str_t str = try_get_id_for_string(
                            config, pred->attr_var, pred->value.frequency_value.content[j].namespace.string);
                        pred->value.frequency_value.content[j].namespace.var = pred->attr_var.var;
                        pred->value.frequency_value.content[j].namespace.str = str;
                    }
                }
                break;
            }
            default:
                switch_default_error("Invalid value type");
                break;
        }
    }
}

bool validate_event(const struct config* config, const struct event* event)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->allow_undefined == false) {
            bool found = false;
            for(size_t j = 0; j < event->pred_count; j++) {
                const struct pred* pred = event->preds[j];
                if(pred->attr_var.var == attr_domain->attr_var.var) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                fprintf(stderr, "Missing attribute: %s\n", attr_domain->attr_var.attr);
                return false;
            }
        }
    }
    return true;
}

