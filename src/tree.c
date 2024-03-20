#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

static void search_cdir_ids(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right,
    const uint64_t* ids,
    size_t sz);

static bool is_id_in(uint64_t id, const uint64_t* ids, size_t sz);


void init_subs_to_eval(struct subs_to_eval* subs)
{
    size_t init = 10;
    subs->subs = bmalloc(init * sizeof(*subs->subs));
    subs->capacity = init;
    subs->count = 0;
}

void init_subs_to_eval_ext(struct subs_to_eval* subs, size_t init)
{
    subs->subs = bmalloc(init * sizeof(*subs->subs));
    subs->capacity = init;
    subs->count = 0;
}

static void add_sub_to_eval(struct betree_sub* sub, struct subs_to_eval* subs)
{
    if(subs->capacity == subs->count) {
        subs->capacity *= 2;
        subs->subs = brealloc(subs->subs, sizeof(*subs->subs) * subs->capacity);
    }

    subs->subs[subs->count] = sub;
    subs->count++;
}

enum short_circuit_e { SHORT_CIRCUIT_PASS, SHORT_CIRCUIT_FAIL, SHORT_CIRCUIT_NONE };

static enum short_circuit_e try_short_circuit(size_t attr_domains_count,
    const struct short_circuit* short_circuit, const uint64_t* undefined)
{
    size_t count = attr_domains_count / 64 + 1;
    for(size_t i = 0; i < count; i++) {
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

bool match_sub(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report* report,
    struct memoize* memoize,
    const uint64_t* undefined)
{
    enum short_circuit_e short_circuit = try_short_circuit(attr_domains_count, &sub->short_circuit, undefined);
    if(short_circuit != SHORT_CIRCUIT_NONE) {
        if(report != NULL) {
            report->shorted++;
        }
        if(short_circuit == SHORT_CIRCUIT_PASS) {
            return true;
        }
        if(short_circuit == SHORT_CIRCUIT_FAIL) {
            return false;
        }
    }
    bool result = match_node(preds, sub->expr, memoize, report);
    return result;
}

bool match_sub_counting(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report_counting* report,
    struct memoize* memoize,
    const uint64_t* undefined)
{
    enum short_circuit_e short_circuit = try_short_circuit(attr_domains_count, &sub->short_circuit, undefined);
    if(short_circuit != SHORT_CIRCUIT_NONE) {
        if(report != NULL) {
            report->shorted++;
        }
        if(short_circuit == SHORT_CIRCUIT_PASS) {
            return true;
        }
        if(short_circuit == SHORT_CIRCUIT_FAIL) {
            return false;
        }
    }
    bool result = match_node_counting(preds, sub->expr, memoize, report);
    return result;
}

static void check_sub(const struct lnode* lnode, struct subs_to_eval* subs)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        add_sub_to_eval(sub, subs);
    }
}

static void check_sub_ids(const struct lnode* lnode, struct subs_to_eval* subs,
    const uint64_t* ids, size_t sz)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        if(is_id_in(sub->id, ids, sz)) {
            add_sub_to_eval(sub, subs);
        }
    }
}

static void check_sub_node_counting(const struct lnode* lnode, struct subs_to_eval* subs, int* node_count)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        add_sub_to_eval(sub, subs);
        ++*node_count;
    }
}

static struct pnode* search_pdir(betree_var_t variable_id, const struct pdir* pdir)
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

static void search_cdir(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right);

static void search_cdir_node_counting(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right, int* node_count);

static bool event_contains_variable(const struct betree_variable** preds, betree_var_t variable_id)
{
    return preds[variable_id] != NULL;
}

void match_be_tree(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct subs_to_eval* subs)
{
    check_sub(cnode->lnode, subs);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir(attr_domains, preds, pnode->cdir, subs, true, true);
            }
        }
    }
}


static void match_be_tree_ids(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct subs_to_eval* subs,
    const uint64_t* ids,
    size_t sz)
{
    check_sub_ids(cnode->lnode, subs, ids, sz);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir_ids(attr_domains, preds, pnode->cdir, subs, true, true, ids, sz);
            }
        }
    }
}

void match_be_tree_node_counting(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct subs_to_eval* subs, int* node_count)
{
    check_sub_node_counting(cnode->lnode, subs, node_count);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir_node_counting(attr_domains, preds, pnode->cdir, subs, true, true, node_count);
            }
            ++*node_count;
        }
        ++*node_count;
    }
}

static bool is_event_enclosed(const struct betree_variable** preds, const struct cdir* cdir, bool open_left, bool open_right)
{
    if(cdir == NULL) {
        return false;
    }
    const struct betree_variable* pred = preds[cdir->attr_var.var];
    if(pred == NULL) {
        return true;
    }
    // No open_left for smin because it's always 0
    switch(pred->value.value_type) {
        case BETREE_BOOLEAN:
            return (cdir->bound.bmin <= pred->value.boolean_value) && (cdir->bound.bmax >= pred->value.boolean_value);
        case BETREE_INTEGER:
            return (open_left || cdir->bound.imin <= pred->value.integer_value) && (open_right || cdir->bound.imax >= pred->value.integer_value);
        case BETREE_FLOAT:
            return (open_left || cdir->bound.fmin <= pred->value.float_value) && (open_right || cdir->bound.fmax >= pred->value.float_value);
        case BETREE_STRING:
            return (cdir->bound.smin <= pred->value.string_value.str) && (open_right || cdir->bound.smax >= pred->value.string_value.str);
        case BETREE_INTEGER_ENUM:
            return (cdir->bound.smin <= pred->value.integer_enum_value.ienum) && (open_right || cdir->bound.smax >= pred->value.integer_enum_value.ienum);
        case BETREE_INTEGER_LIST:
            if(pred->value.integer_list_value->count != 0) {
                int64_t min = pred->value.integer_list_value->integers[0];
                int64_t max = pred->value.integer_list_value->integers[pred->value.integer_list_value->count - 1];
                int64_t bound_min = open_left ? INT64_MIN : cdir->bound.imin;
                int64_t bound_max = open_right ? INT64_MAX : cdir->bound.imax;
                return min <= bound_max && bound_min <= max;
            }
            else {
                return true;
            }
        case BETREE_STRING_LIST:
            if(pred->value.string_list_value->count != 0) {
                size_t min = pred->value.string_list_value->strings[0].str;
                size_t max = pred->value.string_list_value->strings[pred->value.string_list_value->count - 1].str;
                size_t bound_min = cdir->bound.smin;
                size_t bound_max = open_right ? SIZE_MAX : cdir->bound.smax;
                return min <= bound_max && bound_min <= max;
            }
            else {
                return true;
            }
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
            return true;
        default: abort();
    }
    return false;
}

bool sub_is_enclosed(const struct attr_domain** attr_domains, const struct betree_sub* sub, const struct cdir* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(test_bit(sub->attr_vars, cdir->attr_var.var) == true) {
        const struct attr_domain* attr_domain = get_attr_domain(attr_domains, cdir->attr_var.var);
        struct value_bound bound = get_variable_bound(attr_domain, sub->expr);
        switch(attr_domain->bound.value_type) {
            case(BETREE_INTEGER):
            case(BETREE_INTEGER_LIST):
                return cdir->bound.imin <= bound.imin && cdir->bound.imax >= bound.imax;
            case(BETREE_FLOAT): {
                return cdir->bound.fmin <= bound.fmin && cdir->bound.fmax >= bound.fmax;
            }
            case(BETREE_BOOLEAN): {
                return cdir->bound.bmin <= bound.bmin && cdir->bound.bmax >= bound.bmax;
            }
            case(BETREE_STRING):
            case(BETREE_STRING_LIST):
            case(BETREE_INTEGER_ENUM):
                return cdir->bound.smin <= bound.smin && cdir->bound.smax >= bound.smax;
            case(BETREE_SEGMENTS): {
                fprintf(
                    stderr, "%s a segments value cdir should never happen for now\n", __func__);
                abort();
            }
            case(BETREE_FREQUENCY_CAPS): {
                fprintf(stderr,
                    "%s a frequency value cdir should never happen for now\n",
                    __func__);
                abort();
            }
            default: abort();
        }
    }
    return false;
}

static void search_cdir(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right)
{
    match_be_tree(attr_domains, preds, cdir->cnode, subs);
    if(is_event_enclosed(preds, cdir->lchild, open_left, false)) {
        search_cdir(attr_domains, preds, cdir->lchild, subs, open_left, false);
    }
    if(is_event_enclosed(preds, cdir->rchild, false, open_right)) {
        search_cdir(attr_domains, preds, cdir->rchild, subs, false, open_right);
    }
}

static void search_cdir_ids(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right,
    const uint64_t* ids,
    size_t sz)
{
    match_be_tree_ids(attr_domains, preds, cdir->cnode, subs, ids, sz);
    if(is_event_enclosed(preds, cdir->lchild, open_left, false)) {
        search_cdir_ids(attr_domains, preds, cdir->lchild, subs, open_left, false, ids, sz);
    }
    if(is_event_enclosed(preds, cdir->rchild, false, open_right)) {
        search_cdir_ids(attr_domains, preds, cdir->rchild, subs, false, open_right, ids, sz);
    }
}

static void search_cdir_node_counting(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right, int* node_count)
{
    match_be_tree_node_counting(attr_domains, preds, cdir->cnode, subs, node_count);
    if(is_event_enclosed(preds, cdir->lchild, open_left, false)) {
        search_cdir_node_counting(attr_domains, preds, cdir->lchild, subs, open_left, false, node_count);
    }
    if(is_event_enclosed(preds, cdir->rchild, false, open_right)) {
        search_cdir_node_counting(attr_domains, preds, cdir->rchild, subs, false, open_right, node_count);
    }
}

static bool is_used_cnode(betree_var_t variable_id, const struct cnode* cnode);

static bool is_used_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    if(pdir == NULL || pdir->parent == NULL) {
        return false;
    }
    return is_used_cnode(variable_id, pdir->parent);
}

static bool is_used_pnode(betree_var_t variable_id, const struct pnode* pnode)
{
    if(pnode == NULL || pnode->parent == NULL) {
        return false;
    }
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_used_pdir(variable_id, pnode->parent);
}

static bool is_used_cdir(betree_var_t variable_id, const struct cdir* cdir)
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
        default: abort();
    }
}

static bool is_used_cnode(betree_var_t variable_id, const struct cnode* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    if(cnode->parent == NULL) {
        return false;
    }
    return is_used_cdir(variable_id, cnode->parent);
}

static void insert_sub(const struct betree_sub* sub, struct lnode* lnode)
{
    if(lnode->sub_count == 0) {
        lnode->subs = bcalloc(sizeof(*lnode->subs));
        if(lnode->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_sub** subs = brealloc(lnode->subs, sizeof(*subs) * (lnode->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        lnode->subs = subs;
    }
    lnode->subs[lnode->sub_count] = (struct betree_sub*)sub;
    lnode->sub_count++;
}

static bool is_root(const struct cnode* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    return cnode->parent == NULL;
}

static void space_partitioning(const struct config* config, struct cnode* cnode);
static void space_clustering(const struct config* config, struct cdir* cdir);
static struct cdir* insert_cdir(
    const struct config* config, const struct betree_sub* sub, struct cdir* cdir);

static size_t count_attr_in_lnode(betree_var_t variable_id, const struct lnode* lnode);

static size_t count_attr_in_cdir(betree_var_t variable_id, const struct cdir* cdir)
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

static size_t domain_bound_diff(const struct attr_domain* attr_domain)
{
    const struct value_bound* b = &attr_domain->bound;
    switch(b->value_type) {
        case BETREE_BOOLEAN:
            return 1;
        case BETREE_INTEGER:
        case BETREE_INTEGER_LIST:
            if(b->imin == INT64_MIN && b->imax == INT64_MAX) {
                return SIZE_MAX;
            }
            else {
                return (size_t)(llabs(b->imax - b->imin));
            }
        case BETREE_FLOAT:
            if(feq(b->fmin, -DBL_MAX) && feq(b->fmax, DBL_MAX)) {
                return SIZE_MAX;
            }
            else {
                return (size_t)(fabs(b->fmax - b->fmin));
            }
        case BETREE_STRING:
        case BETREE_STRING_LIST:
        case BETREE_INTEGER_ENUM:
            return b->smax - b->smin;
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
        default:
            abort();
    }
}

static double get_attr_domain_score(const struct attr_domain* attr_domain)
{
    size_t diff = domain_bound_diff(attr_domain);
    if(diff == 0) {
        diff = 1;
    }
    double num = attr_domain->allow_undefined ? 1. : 10.;
    double bound_score = num / (double)diff;
    return bound_score;
}

static double get_score(const struct attr_domain** attr_domains, betree_var_t var, size_t count)
{
    const struct attr_domain* attr_domain = get_attr_domain(attr_domains, var);
    double attr_domain_score = get_attr_domain_score(attr_domain);
    double score = (double)count * attr_domain_score;
    return score;
}

static double get_pnode_score(const struct attr_domain** attr_domains, struct pnode* pnode)
{
    size_t count = count_attr_in_cdir(pnode->attr_var.var, pnode->cdir);
    return get_score(attr_domains, pnode->attr_var.var, count);
}

static double get_lnode_score(
    const struct attr_domain** attr_domains, const struct lnode* lnode, betree_var_t var)
{
    size_t count = count_attr_in_lnode(var, lnode);
    return get_score(attr_domains, var, count);
}

static void update_partition_score(const struct attr_domain** attr_domains, struct pnode* pnode)
{
    pnode->score = get_pnode_score(attr_domains, pnode);
}

bool insert_be_tree(
    const struct config* config, const struct betree_sub* sub, struct cnode* cnode, struct cdir* cdir)
{
    if(config == NULL) {
        fprintf(stderr, "Config is NULL, required to insert in the be tree\n");
        abort();
    }
    bool foundPartition = false;
    struct pnode* max_pnode = NULL;
    if(cnode->pdir != NULL) {
        float max_score = -DBL_MAX;
        for(size_t i = 0; i < config->attr_domain_count; i++) {
            if(test_bit(sub->attr_vars, i) == false) {
                continue;
            }
            betree_var_t variable_id = i;
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
        update_partition_score((const struct attr_domain**)config->attr_domains, max_pnode);
    }
    return true;
}

static bool is_leaf(const struct cdir* cdir)
{
    return cdir->lchild == NULL && cdir->rchild == NULL;
}

static struct cdir* insert_cdir(
    const struct config* config, const struct betree_sub* sub, struct cdir* cdir)
{
    if(is_leaf(cdir)) {
        return cdir;
    }
    if(sub_is_enclosed((const struct attr_domain**)config->attr_domains, sub, cdir->lchild)) {
        return insert_cdir(config, sub, cdir->lchild);
    }
    if(sub_is_enclosed((const struct attr_domain**)config->attr_domains, sub, cdir->rchild)) {
        return insert_cdir(config, sub, cdir->rchild);
    }
    return cdir;
}

static bool is_overflowed(const struct lnode* lnode)
{
    return lnode->sub_count > lnode->max;
}

bool sub_has_attribute(const struct betree_sub* sub, betree_var_t variable_id)
{
    return test_bit(sub->attr_vars, variable_id);
}

bool sub_has_attribute_str(struct config* config, const struct betree_sub* sub, const char* attr)
{
    betree_var_t variable_id = try_get_id_for_attr(config, attr);
    if(variable_id == INVALID_VAR) {
        return false;
    }
    return sub_has_attribute(sub, variable_id);
}

static bool remove_sub(betree_sub_t sub, struct lnode* lnode)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* lnode_sub = lnode->subs[i];
        if(sub == lnode_sub->id) {
            for(size_t j = i; j < lnode->sub_count - 1; j++) {
                lnode->subs[j] = lnode->subs[j + 1];
            }
            lnode->sub_count--;
            if(lnode->sub_count == 0) {
                bfree(lnode->subs);
                lnode->subs = NULL;
            }
            else {
                struct betree_sub** subs = brealloc(lnode->subs, sizeof(*lnode->subs) * lnode->sub_count);
                if(subs == NULL) {
                    fprintf(stderr, "%s brealloc failed\n", __func__);
                    abort();
                }
                lnode->subs = subs;
            }
            return true;
        }
    }
    return false;
}

static void move(const struct betree_sub* sub, struct lnode* origin, struct lnode* destination)
{
    bool isFound = remove_sub(sub->id, origin);
    if(!isFound) {
        fprintf(stderr, "Could not find sub %" PRIu64 "\n", sub->id);
        abort();
    }
    if(destination->sub_count == 0) {
        destination->subs = bcalloc(sizeof(*destination->subs));
        if(destination->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_sub** subs
            = brealloc(destination->subs, sizeof(*destination->subs) * (destination->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        destination->subs = subs;
    }
    destination->subs[destination->sub_count] = (struct betree_sub*)sub;
    destination->sub_count++;
}

static struct cdir* create_cdir(const struct config* config,
    const char* attr,
    betree_var_t variable_id,
    struct value_bound bound)
{
    struct cdir* cdir = bcalloc(sizeof(*cdir));
    if(cdir == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    cdir->attr_var.attr = bstrdup(attr);
    cdir->attr_var.var = variable_id;
    cdir->bound = bound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lchild = NULL;
    cdir->rchild = NULL;
    return cdir;
}

static struct cdir* create_cdir_with_cdir_parent(
    const struct config* config, struct cdir* parent, struct value_bound bound)
{
    struct cdir* cdir = create_cdir(config, parent->attr_var.attr, parent->attr_var.var, bound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

static struct cdir* create_cdir_with_pnode_parent(
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
        pdir = bcalloc(sizeof(*pdir));
        if(pdir == NULL) {
            fprintf(stderr, "%s pdir bcalloc failed\n", __func__);
            abort();
        }
        pdir->parent = cnode;
        pdir->pnode_count = 0;
        pdir->pnodes = NULL;
        cnode->pdir = pdir;
    }

    struct pnode* pnode = bcalloc(sizeof(*pnode));
    if(pnode == NULL) {
        fprintf(stderr, "%s pnode bcalloc failed\n", __func__);
        abort();
    }
    pnode->cdir = NULL;
    pnode->parent = pdir;
    pnode->attr_var.attr = bstrdup(attr);
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
        pdir->pnodes = bcalloc(sizeof(*pdir->pnodes));
        if(pdir->pnodes == NULL) {
            fprintf(stderr, "%s pnodes bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct pnode** pnodes = brealloc(pdir->pnodes, sizeof(*pnodes) * (pdir->pnode_count + 1));
        if(pnodes == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        pdir->pnodes = pnodes;
    }
    pdir->pnodes[pdir->pnode_count] = pnode;
    pdir->pnode_count++;
    return pnode;
}

static size_t count_attr_in_lnode(betree_var_t variable_id, const struct lnode* lnode)
{
    size_t count = 0;
    if(lnode == NULL) {
        return count;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        if(sub == NULL) {
            fprintf(stderr, "%s, sub is NULL\n", __func__);
            continue;
        }
        if(test_bit(sub->attr_vars, variable_id) == true) {
            count++;
        }
    }
    return count;
}

static bool is_attr_used_in_parent_cnode(betree_var_t variable_id, const struct cnode* cnode);

static bool is_attr_used_in_parent_pdir(betree_var_t variable_id, const struct pdir* pdir)
{
    return is_attr_used_in_parent_cnode(variable_id, pdir->parent);
}

static bool is_attr_used_in_parent_pnode(betree_var_t variable_id, const struct pnode* pnode)
{
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_attr_used_in_parent_pdir(variable_id, pnode->parent);
}

static bool is_attr_used_in_parent_cdir(betree_var_t variable_id, const struct cdir* cdir)
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
        default: abort();
    }
}

static bool is_attr_used_in_parent_cnode(betree_var_t variable_id, const struct cnode* cnode)
{
    if(is_root(cnode)) {
        return false;
    }
    return is_attr_used_in_parent_cdir(variable_id, cnode->parent);
}

static bool is_attr_used_in_parent_lnode(betree_var_t variable_id, const struct lnode* lnode)
{
    return is_attr_used_in_parent_cnode(variable_id, lnode->parent);
}

static bool splitable_attr_domain(
    const struct config* config, const struct attr_domain* attr_domain)
{
    switch(attr_domain->bound.value_type) {
        case BETREE_INTEGER:
        case BETREE_INTEGER_LIST:
            if(attr_domain->bound.imin == INT64_MIN || attr_domain->bound.imax == INT64_MAX) {
                return false;
            }
            return ((uint64_t)llabs(attr_domain->bound.imax - attr_domain->bound.imin))
                < config->max_domain_for_split;
        case BETREE_FLOAT:
            if(feq(attr_domain->bound.fmin, -DBL_MAX) || feq(attr_domain->bound.fmax, DBL_MAX)) {
                return false;
            }
            return ((uint64_t)fabs(attr_domain->bound.fmax - attr_domain->bound.fmin))
                < config->max_domain_for_split;
        case BETREE_BOOLEAN:
            return true;
        case BETREE_STRING:
        case BETREE_STRING_LIST:
        case BETREE_INTEGER_ENUM:
            if(attr_domain->bound.smax == SIZE_MAX) {
                return false;
            }
            return (attr_domain->bound.smax - attr_domain->bound.smin)
                < config->max_domain_for_split;
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
            return false;
        default: abort();
    }
}

static bool get_next_highest_score_unused_attr(
    const struct config* config, const struct lnode* lnode, betree_var_t* var)
{
    bool found = false;
    double highest_score = 0;
    betree_var_t highest_var;
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        for(size_t j = 0; j < config->attr_domain_count; j++) {
            if(test_bit(sub->attr_vars, j) == false) {
                continue;
            }
            betree_var_t current_variable_id = j;
            const struct attr_domain* attr_domain = get_attr_domain(
                (const struct attr_domain**)config->attr_domains, current_variable_id);
            if(splitable_attr_domain(config, attr_domain)
                && !is_attr_used_in_parent_lnode(current_variable_id, lnode)) {
                double current_score = get_lnode_score(
                    (const struct attr_domain**)config->attr_domains, lnode, current_variable_id);
                found = true;
                if(current_score > highest_score) {
                    highest_score = current_score;
                    highest_var = current_variable_id;
                }
            }
        }
    }
    if(found == false) {
        return false;
    }
    *var = highest_var;
    return true;
}

static void update_cluster_capacity(const struct config* config, struct lnode* lnode)
{
    if(lnode == NULL) {
        return;
    }
    size_t count = lnode->sub_count;
    size_t max = smax(config->lnode_max_cap,
        ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
    lnode->max = max;
}

static size_t count_subs_with_variable(
    const struct betree_sub** subs, size_t sub_count, betree_var_t variable_id)
{
    size_t count = 0;
    for(size_t i = 0; i < sub_count; i++) {
        const struct betree_sub* sub = subs[i];
        if(sub_has_attribute(sub, variable_id)) {
            count++;
        }
    }
    return count;
}

static void space_partitioning(const struct config* config, struct cnode* cnode)
{
    struct lnode* lnode = cnode->lnode;
    while(is_overflowed(lnode) == true) {
        betree_var_t var;
        bool found = get_next_highest_score_unused_attr(config, lnode, &var);
        if(found == false) {
            break;
        }
        size_t target_subs_count = count_subs_with_variable(
            (const struct betree_sub**)lnode->subs, lnode->sub_count, var);
        if(target_subs_count < config->partition_min_size) {
            break;
        }
        const char* attr = config->attr_domains[var]->attr_var.attr;
        struct pnode* pnode = create_pdir(config, attr, var, cnode);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            const struct betree_sub* sub = lnode->subs[i];
            if(sub_has_attribute(sub, var)) {
                struct cdir* cdir = insert_cdir(config, sub, pnode->cdir);
                move(sub, lnode, cdir->cnode->lnode);
                i--;
            }
        }
        space_clustering(config, pnode->cdir);
    }
    update_cluster_capacity(config, lnode);
}

static bool is_atomic(const struct cdir* cdir)
{
    switch(cdir->bound.value_type) {
        case(BETREE_INTEGER):
        case(BETREE_INTEGER_LIST):
            return cdir->bound.imin == cdir->bound.imax;
        case(BETREE_FLOAT): {
            return feq(cdir->bound.fmin, cdir->bound.fmax);
        }
        case(BETREE_BOOLEAN): {
            return cdir->bound.bmin == cdir->bound.bmax;
        }
        case(BETREE_STRING):
        case(BETREE_STRING_LIST):
        case(BETREE_INTEGER_ENUM):
            return cdir->bound.smin == cdir->bound.smax;
        case(BETREE_SEGMENTS): {
            fprintf(stderr, "%s a segments value cdir should never happen for now\n", __func__);
            abort();
        }
        case(BETREE_FREQUENCY_CAPS): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: abort();
    }
}

struct lnode* make_lnode(const struct config* config, struct cnode* parent)
{
    struct lnode* lnode = bcalloc(sizeof(*lnode));
    if(lnode == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
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
    struct cnode* cnode = bcalloc(sizeof(*cnode));
    if(cnode == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
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

static struct value_bounds split_value_bound(struct value_bound bound)
{
    struct value_bound lbound = { .value_type = bound.value_type };
    struct value_bound rbound = { .value_type = bound.value_type };
    switch(bound.value_type) {
        case(BETREE_INTEGER):
        case(BETREE_INTEGER_LIST): {
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
        case(BETREE_FLOAT): {
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
        case(BETREE_BOOLEAN): {
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
        case(BETREE_STRING):
        case(BETREE_STRING_LIST): 
        case(BETREE_INTEGER_ENUM): {
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
        case(BETREE_SEGMENTS): {
            fprintf(stderr, "%s a segment value cdir should never happen for now\n", __func__);
            abort();
        }
        case(BETREE_FREQUENCY_CAPS): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: abort();
    }
    struct value_bounds bounds = { .lbound = lbound, .rbound = rbound };
    return bounds;
}

static void space_clustering(const struct config* config, struct cdir* cdir)
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
            const struct betree_sub* sub = lnode->subs[i];
            if(sub_is_enclosed((const struct attr_domain**)config->attr_domains, sub, cdir->lchild)) {
                move(sub, lnode, cdir->lchild->cnode->lnode);
                i--;
            }
            else if(sub_is_enclosed((const struct attr_domain**)config->attr_domains, sub, cdir->rchild)) {
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

/*static bool search_delete_cdir(size_t attr_domains_count,*/
    /*const struct attr_domain** attr_domains, struct betree_sub* sub, struct cdir* cdir);*/

/*static bool delete_sub_from_leaf(betree_sub_t sub, struct lnode* lnode)*/
/*{*/
    /*return remove_sub(sub, lnode);*/
/*}*/

/*static bool is_lnode_empty(const struct lnode* lnode)*/
/*{*/
    /*return lnode == NULL || lnode->sub_count == 0;*/
/*}*/

/*static bool is_pdir_empty(const struct pdir* pdir)*/
/*{*/
    /*return pdir == NULL || pdir->pnode_count == 0;*/
/*}*/

/*static bool is_cnode_empty(const struct cnode* cnode)*/
/*{*/
    /*return cnode == NULL || (is_lnode_empty(cnode->lnode) && is_pdir_empty(cnode->pdir));*/
/*}*/

/*static bool is_cdir_empty(const struct cdir* cdir)*/
/*{*/
    /*return cdir == NULL*/
        /*|| (is_cnode_empty(cdir->cnode) && is_cdir_empty(cdir->lchild)*/
               /*&& is_cdir_empty(cdir->rchild));*/
/*}*/

/*static bool is_pnode_empty(const struct pnode* pnode)*/
/*{*/
    /*return pnode == NULL || (is_cdir_empty(pnode->cdir));*/
/*}*/

static void free_pnode(struct pnode* pnode);

static void free_pdir(struct pdir* pdir)
{
    if(pdir == NULL) {
        return;
    }
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        struct pnode* pnode = pdir->pnodes[i];
        free_pnode(pnode);
    }
    bfree(pdir->pnodes);
    pdir->pnodes = NULL;
    bfree(pdir);
}

static void free_pred(struct betree_variable* pred)
{
    if(pred == NULL) {
        return;
    }
    bfree((char*)pred->attr_var.attr);
    free_value(pred->value);
    bfree(pred);
}

void free_sub(struct betree_sub* sub)
{
    if(sub == NULL) {
        return;
    }
    bfree(sub->attr_vars);
    sub->attr_vars = NULL;
    free_ast_node((struct ast_node*)sub->expr);
    sub->expr = NULL;
    bfree(sub->short_circuit.pass);
    bfree(sub->short_circuit.fail);
    bfree(sub);
}

void free_event(struct betree_event* event)
{
    if(event == NULL) {
        return;
    }
    for(size_t i = 0; i < event->variable_count; i++) {
        const struct betree_variable* pred = event->variables[i];
        if(pred != NULL) {
            free_pred((struct betree_variable*)pred);
        }
    }
    bfree(event->variables);
    bfree(event);
}

void free_lnode(struct lnode* lnode)
{
    if(lnode == NULL) {
        return;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        free_sub((struct betree_sub*)sub);
    }
    bfree(lnode->subs);
    lnode->subs = NULL;
    bfree(lnode);
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
    bfree(cnode);
}

static void free_cdir(struct cdir* cdir)
{
    if(cdir == NULL) {
        return;
    }
    bfree((char*)cdir->attr_var.attr);
    free_cnode(cdir->cnode);
    cdir->cnode = NULL;
    free_cdir(cdir->lchild);
    cdir->lchild = NULL;
    free_cdir(cdir->rchild);
    cdir->rchild = NULL;
    bfree(cdir);
}

/*static void try_remove_pnode_from_parent(const struct pnode* pnode)*/
/*{*/
    /*struct pdir* pdir = pnode->parent;*/
    /*for(size_t i = 0; i < pdir->pnode_count; i++) {*/
        /*if(pnode == pdir->pnodes[i]) {*/
            /*for(size_t j = i; j < pdir->pnode_count - 1; j++) {*/
                /*pdir->pnodes[j] = pdir->pnodes[j + 1];*/
            /*}*/
            /*pdir->pnode_count--;*/
            /*if(pdir->pnode_count == 0) {*/
                /*bfree(pdir->pnodes);*/
                /*pdir->pnodes = NULL;*/
            /*}*/
            /*else {*/
                /*struct pnode** pnodes = brealloc(pdir->pnodes, sizeof(*pnodes) * pdir->pnode_count);*/
                /*if(pnodes == NULL) {*/
                    /*fprintf(stderr, "%s brealloc failed\n", __func__);*/
                    /*abort();*/
                /*}*/
                /*pdir->pnodes = pnodes;*/
            /*}*/
            /*return;*/
        /*}*/
    /*}*/
/*}*/

static void free_pnode(struct pnode* pnode)
{
    if(pnode == NULL) {
        return;
    }
    bfree((char*)pnode->attr_var.attr);
    free_cdir(pnode->cdir);
    pnode->cdir = NULL;
    bfree(pnode);
}

/*bool betree_delete_inner(size_t attr_domains_count,*/
    /*const struct attr_domain** attr_domains, struct betree_sub* sub, struct cnode* cnode)*/
/*{*/
    /*struct pnode* pnode = NULL;*/
    /*bool isFound = delete_sub_from_leaf(sub->id, cnode->lnode);*/
    /*if(!isFound) {*/
        /*for(size_t i = 0; i < attr_domains_count; i++) {*/
            /*if(test_bit(sub->attr_vars, i) == false) {*/
                /*continue;*/
            /*}*/
            /*betree_var_t variable_id = i;*/
            /*pnode = search_pdir(variable_id, cnode->pdir);*/
            /*if(pnode != NULL) {*/
                /*isFound = search_delete_cdir(attr_domains_count, attr_domains, sub, pnode->cdir);*/
            /*}*/
            /*if(isFound) {*/
                /*break;*/
            /*}*/
        /*}*/
    /*}*/
    /*if(isFound) {*/
        /*if(pnode != NULL && is_pnode_empty(pnode)) {*/
            /*try_remove_pnode_from_parent(pnode);*/
            /*free_pnode(pnode);*/
        /*}*/
        /*if(cnode != NULL && is_pdir_empty(cnode->pdir)) {*/
            /*free_pdir(cnode->pdir);*/
            /*cnode->pdir = NULL;*/
        /*}*/
        /*if(!is_root(cnode)) {*/
            /*if(cnode != NULL && is_lnode_empty(cnode->lnode)) {*/
                /*free_lnode(cnode->lnode);*/
                /*cnode->lnode = NULL;*/
            /*}*/
            /*if(is_cnode_empty(cnode)) {*/
                /*cnode->parent->cnode = NULL;*/
                /*free_cnode(cnode);*/
            /*}*/
        /*}*/
    /*}*/
    /*return isFound;*/
/*}*/

static struct betree_sub* find_sub_id_cdir(betree_sub_t id, struct cdir* cdir)
{
    if(cdir == NULL) {
        return NULL;
    }
    struct betree_sub* in_cnode = find_sub_id(id, cdir->cnode);
    if(in_cnode != NULL) {
        return in_cnode;
    }
    struct betree_sub* in_lcdir = find_sub_id_cdir(id, cdir->lchild);
    if(in_lcdir != NULL) {
        return in_lcdir;
    }
    struct betree_sub* in_rcdir = find_sub_id_cdir(id, cdir->rchild);
    if(in_rcdir != NULL) {
        return in_rcdir;
    }
    return NULL;
}

struct betree_sub* find_sub_id(betree_sub_t id, struct cnode* cnode)
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
            struct betree_sub* in_cdir = find_sub_id_cdir(id, cnode->pdir->pnodes[i]->cdir);
            if(in_cdir != NULL) {
                return in_cdir;
            }
        }
    }
    return NULL;
}

/*static bool is_empty(struct cdir* cdir)*/
/*{*/
    /*return is_cdir_empty(cdir);*/
/*}*/

/*static void remove_bucket(struct cdir* cdir)*/
/*{*/
    /*free_cdir(cdir);*/
/*}*/

/*static void try_remove_cdir_from_parent(struct cdir* cdir)*/
/*{*/
    /*switch(cdir->parent_type) {*/
        /*case CNODE_PARENT_CDIR: {*/
            /*if(cdir->cdir_parent->lchild == cdir) {*/
                /*cdir->cdir_parent->lchild = NULL;*/
            /*}*/
            /*else if(cdir->cdir_parent->rchild == cdir) {*/
                /*cdir->cdir_parent->rchild = NULL;*/
            /*}*/
            /*break;*/
        /*}*/
        /*case CNODE_PARENT_PNODE: {*/
            /*cdir->pnode_parent->cdir = NULL;*/
            /*break;*/
        /*}*/
        /*default: {*/
            /*switch_default_error("Invalid cdir parent type");*/
        /*}*/
    /*}*/
/*}*/

/*static bool search_delete_cdir(size_t attr_domains_count,*/
    /*const struct attr_domain** attr_domains, struct betree_sub* sub, struct cdir* cdir)*/
/*{*/
    /*bool isFound = false;*/
    /*if(sub_is_enclosed(attr_domains, sub, cdir->lchild)) {*/
        /*isFound = search_delete_cdir(attr_domains_count, attr_domains, sub, cdir->lchild);*/
    /*}*/
    /*else if(sub_is_enclosed(attr_domains, sub, cdir->rchild)) {*/
        /*isFound = search_delete_cdir(attr_domains_count, attr_domains, sub, cdir->rchild);*/
    /*}*/
    /*else {*/
        /*isFound = betree_delete_inner(attr_domains_count, attr_domains, sub, cdir->cnode);*/
    /*}*/
    /*if(isFound) {*/
        /*if(is_empty(cdir->lchild)) {*/
            /*remove_bucket(cdir->lchild);*/
            /*cdir->lchild = NULL;*/
        /*}*/
        /*if(is_empty(cdir->rchild)) {*/
            /*remove_bucket(cdir->rchild);*/
            /*cdir->rchild = NULL;*/
        /*}*/
        /*if(is_empty(cdir)) {*/
            /*try_remove_cdir_from_parent(cdir);*/
            /*free_cdir(cdir);*/
        /*}*/
    /*}*/
    /*return isFound;*/
/*}*/

struct betree_variable* make_pred(const char* attr, betree_var_t variable_id, struct value value)
{
    struct betree_variable* pred = bcalloc(sizeof(*pred));
    if(pred == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    pred->attr_var.attr = bstrdup(attr);
    pred->attr_var.var = variable_id;
    pred->value = value;
    return pred;
}

static void fill_pred_attr_var(struct betree_sub* sub, struct attr_var attr_var)
{
    set_bit(sub->attr_vars, attr_var.var);
}

void fill_pred(struct betree_sub* sub, const struct ast_node* expr)
{
    switch(expr->type) {
        case AST_TYPE_IS_NULL_EXPR:
            fill_pred_attr_var(sub, expr->is_null_expr.attr_var);
            return;
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
                default: abort();
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
                case AST_BOOL_LITERAL:
                    return;
                default: abort();
            }
            return;
        }
        case AST_TYPE_COMPARE_EXPR: {
            fill_pred_attr_var(sub, expr->compare_expr.attr_var);
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
        default: abort();
    }
}

static enum short_circuit_e short_circuit_for_attr_var(
    betree_var_t id, bool inverted, struct attr_var attr_var)
{
    if(id == attr_var.var) {
        if(inverted) {
            return SHORT_CIRCUIT_PASS;
        }
        return SHORT_CIRCUIT_FAIL;
    }
    return SHORT_CIRCUIT_NONE;
}

static enum short_circuit_e short_circuit_for_node(
    betree_var_t id, bool inverted, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            switch(node->is_null_expr.type) {
                case AST_IS_NULL:
                    return short_circuit_for_attr_var(id, !inverted, node->is_null_expr.attr_var);
                case AST_IS_NOT_NULL:
                    return short_circuit_for_attr_var(id, inverted, node->is_null_expr.attr_var);
                case AST_IS_EMPTY:
                    return short_circuit_for_attr_var(id, inverted, node->is_null_expr.attr_var);
                default:
                    abort();
            }
        case AST_TYPE_COMPARE_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->compare_expr.attr_var);
        case AST_TYPE_EQUALITY_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->equality_expr.attr_var);
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL:
                    return SHORT_CIRCUIT_NONE;
                case AST_BOOL_OR: {
                    enum short_circuit_e lhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
                    if(lhs == SHORT_CIRCUIT_PASS || rhs == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    if(lhs == SHORT_CIRCUIT_FAIL && rhs == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_BOOL_AND: {
                    enum short_circuit_e lhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
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
                return short_circuit_for_attr_var(
                    id, inverted, node->set_expr.left_value.variable_value);
            }
            else {
                return short_circuit_for_attr_var(
                    id, inverted, node->set_expr.right_value.variable_value);
            }
        case AST_TYPE_LIST_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->list_expr.attr_var);
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    enum short_circuit_e frequency = short_circuit_for_attr_var(id, inverted, node->special_expr.frequency.attr_var);
                    enum short_circuit_e now = short_circuit_for_attr_var(id, inverted, node->special_expr.frequency.now);
                    if(frequency == SHORT_CIRCUIT_FAIL || now == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(frequency == SHORT_CIRCUIT_PASS && now == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_SEGMENT: {
                    enum short_circuit_e frequency = short_circuit_for_attr_var(id, inverted, node->special_expr.segment.attr_var);
                    enum short_circuit_e now = short_circuit_for_attr_var(id, inverted, node->special_expr.segment.now);
                    if(frequency == SHORT_CIRCUIT_FAIL || now == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(frequency == SHORT_CIRCUIT_PASS && now == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_GEO: {
                    enum short_circuit_e latitude = short_circuit_for_attr_var(id, inverted, node->special_expr.geo.latitude_var);
                    enum short_circuit_e longitude = short_circuit_for_attr_var(id, inverted, node->special_expr.geo.longitude_var);
                    if(latitude == SHORT_CIRCUIT_FAIL || longitude == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(latitude == SHORT_CIRCUIT_PASS && longitude == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_STRING:
                    return short_circuit_for_attr_var(
                        id, inverted, node->special_expr.string.attr_var);
                default:
                    abort();
            }
        default:
            abort();
    }
    return SHORT_CIRCUIT_NONE;
}

static void fill_short_circuit(struct config* config, struct betree_sub* sub)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->allow_undefined) {
            enum short_circuit_e result
                = short_circuit_for_node(attr_domain->attr_var.var, false, sub->expr);
            if(result == SHORT_CIRCUIT_PASS) {
                set_bit(sub->short_circuit.pass, i);
            }
            else if(result == SHORT_CIRCUIT_FAIL) {
                set_bit(sub->short_circuit.fail, i);
            }
        }
    }
}

struct betree_sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr)
{
    struct betree_sub* sub = bcalloc(sizeof(*sub));
    if(sub == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    sub->id = id;
    size_t count = config->attr_domain_count / 64 + 1;
    sub->attr_vars = bcalloc(count * sizeof(*sub->attr_vars));
    sub->expr = expr;
    fill_pred(sub, sub->expr);
    sub->short_circuit.pass = bcalloc(count * sizeof(*sub->short_circuit.pass));
    sub->short_circuit.fail = bcalloc(count * sizeof(*sub->short_circuit.fail));
    fill_short_circuit(config, sub);
    return sub;
}

struct betree_event* make_empty_event()
{
    struct betree_event* event = bcalloc(sizeof(*event));
    if(event == NULL) {
        fprintf(stderr, "%s event bcalloc failed\n", __func__);
        abort();
    }
    event->variable_count = 0;
    event->variables = NULL;
    return event;
}

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id)
{
    if(variable_id < config->attr_domain_count) {
        return config->attr_domains[variable_id]->attr_var.attr;
    }
    return NULL;
}

betree_var_t try_get_id_for_attr(const struct config* config, const char* attr)
{
    char* copy = bstrdup(attr);
    for(size_t i = 0; copy[i]; i++) {
        copy[i] = tolower(copy[i]);
    }
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        if(strcmp(config->attr_domains[i]->attr_var.attr, copy) == 0) {
            bfree(copy);
            return i;
        }
    }
    bfree(copy);
    return INVALID_VAR;
}

void event_to_string(const struct betree_event* event, char* buffer)
{
    size_t length = 0;
    for(size_t i = 0; i < event->variable_count; i++) {
        const struct betree_variable* pred = event->variables[i];
        if(i != 0) {
            length += sprintf(buffer + length, ", ");
        }
        const char* attr = pred->attr_var.attr;
        switch(pred->value.value_type) {
            case(BETREE_INTEGER): {
                length += sprintf(buffer + length, "%s = %" PRIu64, attr, pred->value.integer_value);
                break;
            }
            case(BETREE_FLOAT): {
                length += sprintf(buffer + length, "%s = %.2f", attr, pred->value.float_value);
                break;
            }
            case(BETREE_BOOLEAN): {
                const char* value = pred->value.boolean_value ? "true" : "false";
                length += sprintf(buffer + length, "%s = %s", attr, value);
                break;
            }
            case(BETREE_STRING): {
                length += sprintf(buffer + length, "%s = \"%s\"", attr, pred->value.string_value.string);
                break;
            }
            case(BETREE_INTEGER_ENUM): {
                length += sprintf(buffer + length, "%s = %ld", attr, pred->value.integer_enum_value.integer);
                break;
            }
            case(BETREE_INTEGER_LIST): {
                const char* integer_list = integer_list_value_to_string(pred->value.integer_list_value);
                length += sprintf(buffer + length, "%s = (%s)", attr, integer_list);
                bfree((char*)integer_list);
                break;
            }
            case(BETREE_SEGMENTS):
            case(BETREE_FREQUENCY_CAPS): {
                fprintf(stderr, "TODO\n");
                abort();
                break;
            }
            case(BETREE_STRING_LIST): {
                const char* string_list = string_list_value_to_string(pred->value.string_list_value);
                length += sprintf(buffer + length, "%s = (%s)", attr, string_list);
                bfree((char*)string_list);
                break;
            }
            default: abort();
        }
    }
    buffer[length] = '\0';
}

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct betree_event** event);

struct memoize make_memoize(size_t pred_count)
{
    size_t count = pred_count / 64 + 1;
    struct memoize memoize = {
        .pass = bcalloc(count * sizeof(*memoize.pass)),
        .fail = bcalloc(count * sizeof(*memoize.fail)),
    };
    return memoize;
}

struct memoize make_memoize_with_count(size_t pred_count, size_t* count)
{
    *count = pred_count / 64 + 1;
    struct memoize memoize = {
        .pass = bcalloc(*count * sizeof(*memoize.pass)),
        .fail = bcalloc(*count * sizeof(*memoize.fail)),
    };
    return memoize;
}

void free_memoize(struct memoize memoize)
{
    bfree(memoize.pass);
    bfree(memoize.fail);
}

static uint64_t* make_undefined(size_t attr_domain_count, const struct betree_variable** preds)
{
    size_t count = attr_domain_count / 64 + 1;
    uint64_t* undefined = bcalloc(count * sizeof(*undefined));
    for(size_t i = 0; i < attr_domain_count; i++) {
        if(preds[i] == NULL) {
            set_bit(undefined, i);
        }
    }
    return undefined;
}

uint64_t* make_undefined_with_count(size_t attr_domain_count, const struct betree_variable** preds, size_t* count)
{
    *count = attr_domain_count / 64 + 1;
    uint64_t* undefined = bcalloc(*count * sizeof(*undefined));
    for(size_t i = 0; i < attr_domain_count; i++) {
        if(preds[i] == NULL) {
            set_bit(undefined, i);
        }
    }
    return undefined;
}

static void add_sub(betree_sub_t id, struct report* report)
{
    if(report->matched == 0) {
        report->subs = bcalloc(sizeof(*report->subs));
        if(report->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        betree_sub_t* subs
            = brealloc(report->subs, sizeof(*report->subs) * (report->matched + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        report->subs = subs;
    }
    report->subs[report->matched] = id;
    report->matched++;
}

void add_sub_counting(betree_sub_t id, struct report_counting* report)
{
    if(report->matched == 0) {
        report->subs = bcalloc(sizeof(*report->subs));
        if(report->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        betree_sub_t* subs
            = brealloc(report->subs, sizeof(*report->subs) * (report->matched + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        report->subs = subs;
    }
    report->subs[report->matched] = id;
    report->matched++;
}

bool betree_search_with_preds(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct report* report)
{
    uint64_t* undefined = make_undefined(config->attr_domain_count, preds);
    struct memoize memoize = make_memoize(config->pred_map->memoize_count);
    struct subs_to_eval subs;
    init_subs_to_eval(&subs);
    match_be_tree((const struct attr_domain**)config->attr_domains, preds, cnode, &subs);
    for(size_t i = 0; i < subs.count; i++) {
        const struct betree_sub* sub = subs.subs[i];
        report->evaluated++;
        if(match_sub(config->attr_domain_count, preds, sub, report, &memoize, undefined) == true) {
            add_sub(sub->id, report);
        }
    }
    bfree(subs.subs);
    free_memoize(memoize);
    bfree(undefined);
    bfree(preds);
    return true;
}


static bool is_id_in(uint64_t id, const uint64_t* ids, size_t sz) {
    if (sz == 0) {
        return false;
    }
    size_t first = 0;
    size_t last = sz - 1;
    if (id < ids[first] || id > ids[last]) {
        return false;
    }
    size_t middle = (first + last)/2;
    while (first <= last) {
        if (id == ids[middle]) {
            return true;
        }
        if (ids[middle] < id) {
            first = middle + 1;
        } else {
            last = middle - 1;
        }
        middle = (first + last)/2;
    }
    return false;
}


bool betree_search_with_preds_ids(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct report* report,
    const uint64_t* ids,
    size_t sz
    )
{
    uint64_t* undefined = make_undefined(config->attr_domain_count, preds);
    struct memoize memoize = make_memoize(config->pred_map->memoize_count);
    struct subs_to_eval subs;
    init_subs_to_eval(&subs);
    match_be_tree_ids((const struct attr_domain**)config->attr_domains, preds, cnode, &subs, ids, sz);
    for(size_t i = 0; i < subs.count; i++) {
        const struct betree_sub* sub = subs.subs[i];
        report->evaluated++;
        if(match_sub(config->attr_domain_count, preds, sub, report, &memoize, undefined) == true) {
            add_sub(sub->id, report);
        }
    }
    bfree(subs.subs);
    free_memoize(memoize);
    bfree(undefined);
    bfree(preds);
    return true;
}

bool betree_exists_with_preds(const struct config* config, const struct betree_variable** preds, const struct cnode* cnode)
{
    uint64_t* undefined = make_undefined(config->attr_domain_count, preds);
    struct memoize memoize = make_memoize(config->pred_map->memoize_count);
    struct subs_to_eval subs;
    init_subs_to_eval(&subs);
    match_be_tree((const struct attr_domain**)config->attr_domains, preds, cnode, &subs);
    bool result = false;
    for(size_t i = 0; i < subs.count; i++) {
        const struct betree_sub* sub = subs.subs[i];
        if(match_sub(config->attr_domain_count, preds, sub, NULL, &memoize, undefined) == true) {
            result = true;
            break;
        }
    }
    bfree(subs.subs);
    free_memoize(memoize);
    bfree(undefined);
    bfree(preds);
    return result;
}

void sort_event_lists(struct betree_event* event)
{
    for(size_t i = 0; i < event->variable_count; i++) {
        struct betree_variable* pred = event->variables[i];
        if(pred == NULL) {
            continue;
        }
        if(pred->value.value_type == BETREE_INTEGER_LIST) {
            sort_and_remove_duplicate_integer_list(pred->value.integer_list_value);
        }
        else if(pred->value.value_type == BETREE_STRING_LIST) {
            sort_and_remove_duplicate_string_list(pred->value.string_list_value);
        }
    }
}

struct betree_event* make_event_from_string(const struct betree* betree, const char* event_str)
{
    struct betree_event* event;
    if(likely(event_parse(event_str, &event))) {
        fprintf(stderr, "Failed to parse event: %s\n", event_str);
        abort();
    }
    fill_event(betree->config, event);
    sort_event_lists(event);
    return event;
}

struct attr_var make_attr_var(const char* attr, struct config* config)
{
    struct attr_var attr_var;
    attr_var.attr = attr == NULL ? NULL : bstrdup(attr);
    if(config == NULL) {
        attr_var.var = INVALID_VAR;
    }
    else {
        attr_var.var = try_get_id_for_attr(config, attr);
    }
    return attr_var;
}

void free_attr_var(struct attr_var attr_var)
{
    bfree((char*)attr_var.attr);
}

struct attr_var copy_attr_var(struct attr_var attr_var)
{
    struct attr_var copy = { .attr = bstrdup(attr_var.attr), .var = attr_var.var };
    return copy;
}

void add_variable(struct betree_variable* variable, struct betree_event* event)
{
    if(variable == NULL) {
        return;
    }
    if(event->variable_count == 0) {
        event->variables = bcalloc(sizeof(*event->variables));
        if(event->variables == NULL) {
            fprintf(stderr, "%s bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_variable** variables
            = brealloc(event->variables, sizeof(*variables) * (event->variable_count + 1));
        if(variables == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        event->variables = variables;
    }
    event->variables[event->variable_count] = variable;
    event->variable_count++;
}

void fill_event(const struct config* config, struct betree_event* event)
{
    for(size_t i = 0; i < event->variable_count; i++) {
        struct betree_variable* pred = event->variables[i];
        if(pred == NULL) {
            continue;
        }
        betree_var_t var = try_get_id_for_attr(config, pred->attr_var.attr);
        if(unlikely(var == INVALID_VAR)) {
            fprintf(stderr, "Cannot find variable %s in config, aborting", pred->attr_var.attr);
            abort();
        }
        pred->attr_var.var = var;
        struct attr_domain* domain = config->attr_domains[var];
        pred->value.value_type = domain->bound.value_type;
        switch(pred->value.value_type) {
            case BETREE_BOOLEAN:
            case BETREE_INTEGER:
            case BETREE_FLOAT:
            case BETREE_INTEGER_LIST:
            case BETREE_SEGMENTS:
                break;
            case BETREE_INTEGER_ENUM: {
                betree_ienum_t ienum
                    = try_get_id_for_ienum(config, pred->attr_var, pred->value.integer_enum_value.integer);
                pred->value.integer_enum_value.var = pred->attr_var.var;
                pred->value.integer_enum_value.ienum = ienum;
                break;
            }
            case BETREE_STRING: {
                betree_str_t str
                    = try_get_id_for_string(config, pred->attr_var, pred->value.string_value.string);
                pred->value.string_value.var = pred->attr_var.var;
                pred->value.string_value.str = str;
                break;
            }
            case BETREE_STRING_LIST: {
                for(size_t j = 0; j < pred->value.string_list_value->count; j++) {
                    betree_str_t str = try_get_id_for_string(
                        config, pred->attr_var, pred->value.string_list_value->strings[j].string);
                    pred->value.string_list_value->strings[j].var = pred->attr_var.var;
                    pred->value.string_list_value->strings[j].str = str;
                }
                break;
            }
            case BETREE_FREQUENCY_CAPS: {
                for(size_t j = 0; j < pred->value.frequency_caps_value->size; j++) {
                    betree_str_t str = try_get_id_for_string(config,
                        pred->attr_var,
                        pred->value.frequency_caps_value->content[j]->namespace.string);
                    pred->value.frequency_caps_value->content[j]->namespace.var = pred->attr_var.var;
                    pred->value.frequency_caps_value->content[j]->namespace.str = str;
                }
                break;
            }
            default: abort();
        }
    }
}

bool validate_variables(const struct config* config, const struct betree_variable* variables[])
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        const struct betree_variable* variable = variables[i];
        if(attr_domain->allow_undefined == false && variable == NULL) {
            return false;
        }
    }
    return true;
}

