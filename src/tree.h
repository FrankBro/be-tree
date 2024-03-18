#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "betree.h"
#include "config.h"
#include "memoize.h"
#include "value.h"

struct betree_variable {
    struct attr_var attr_var;
    struct value value;
};

struct short_circuit {
    uint64_t* pass;
    uint64_t* fail;
};

struct betree_sub {
    betree_sub_t id;
    uint64_t* attr_vars;
    const struct ast_node* expr;
    struct short_circuit short_circuit;
};

struct cnode;

struct lnode {
    struct cnode* parent;
    struct {
        size_t sub_count;
        struct betree_sub** subs;
    };
    size_t max;
};

struct pdir;
struct pnode;
struct cdir;

struct cnode {
    struct cdir* parent;
    struct lnode* lnode;
    struct pdir* pdir;
};

struct cdir;
struct pdir;

struct pnode {
    struct pdir* parent;
    struct attr_var attr_var;
    struct cdir* cdir;
    float score;
};

enum c_parent_e {
    CNODE_PARENT_PNODE,
    CNODE_PARENT_CDIR,
};

struct cdir {
    enum c_parent_e parent_type;
    union {
        struct pnode* pnode_parent;
        struct cdir* cdir_parent;
    };
    struct attr_var attr_var;
    struct value_bound bound;
    struct cnode* cnode;
    struct cdir* lchild;
    struct cdir* rchild;
};

struct pdir {
    struct cnode* parent;
    struct {
        size_t pnode_count;
        struct pnode** pnodes;
    };
};

struct subs_to_eval {
    struct betree_sub** subs;
    size_t capacity;
    size_t count;
};

void init_subs_to_eval(struct subs_to_eval* subs);
void init_subs_to_eval_ext(struct subs_to_eval* subs, size_t init);
uint64_t* make_undefined_with_count(size_t attr_domain_count, const struct betree_variable** preds, size_t* count);
struct memoize make_memoize_with_count(size_t pred_count, size_t* count);
void match_be_tree(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct subs_to_eval* subs);
void match_be_tree_node_counting(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct subs_to_eval* subs, int* node_count);
bool match_sub(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report* report,
    struct memoize* memoize,
    const uint64_t* undefined);
bool match_sub_counting(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report_counting* report,
    struct memoize* memoize,
    const uint64_t* undefined);
void add_sub_counting(betree_sub_t id, struct report_counting* report);

void free_sub(struct betree_sub* sub);
void free_event(struct betree_event* event);

bool sub_has_attribute(const struct betree_sub* sub, betree_var_t variable_id);
bool sub_has_attribute_str(struct config* config, const struct betree_sub* sub, const char* attr);
bool sub_is_enclosed(const struct attr_domain** attr_domains, const struct betree_sub* sub, const struct cdir* cdir);

struct lnode* make_lnode(const struct config* config, struct cnode* parent);
void free_lnode(struct lnode* lnode);
struct cnode* make_cnode(const struct config* config, struct cdir* parent);
void free_cnode(struct cnode* cnode);

void fill_pred(struct betree_sub* sub, const struct ast_node* expr);
struct betree_sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr);
struct betree_event* make_empty_event();
void event_to_string(const struct betree_event* event, char* buffer);

struct betree_variable* make_pred(const char* attr, betree_var_t variable_id, struct value value);
void add_variable(struct betree_variable* variable, struct betree_event* event);

void fill_event(const struct config* config, struct betree_event* event);
bool validate_variables(const struct config* config, const struct betree_variable* variables[]);

struct betree_event* make_event_from_string(const struct betree* betree, const char* event_str);

struct memoize make_memoize(size_t pred_count);
void free_memoize(struct memoize memoize);

struct betree_constant {
    const char* name;
    struct value value;
};

//bool betree_delete_inner(size_t attr_domains_count, const struct attr_domain** attr_domains, struct betree_sub* sub, struct cnode* cnode);
struct betree_sub* find_sub_id(betree_sub_t id, struct cnode* cnode);

bool betree_search_with_preds(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode* cnode,
    struct report* report);
bool betree_exists_with_preds(const struct config* config, const struct betree_variable** preds, const struct cnode* cnode);

bool insert_be_tree(const struct config* config, const struct betree_sub* sub, struct cnode* cnode, struct cdir* cdir);

void sort_event_lists(struct betree_event* event);

