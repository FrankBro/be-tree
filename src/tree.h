#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "betree.h"
#include "config.h"
#include "memoize.h"
#include "value.h"

struct pred {
    struct attr_var attr_var;
    struct value value;
};

struct short_circuit {
    size_t count;
    uint64_t* pass;
    uint64_t* fail;
};

struct sub {
    betree_sub_t id;
    struct {
        size_t attr_var_count;
        struct attr_var* attr_vars;
    };
    const struct ast_node* expr;
    struct short_circuit short_circuit;
};

struct event {
    size_t pred_count;
    struct pred** preds;
};

struct cnode;

struct lnode {
    struct cnode* parent;
    struct {
        size_t sub_count;
        struct sub** subs;
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

void free_sub(struct sub* sub);
void free_event(struct event* event);

bool sub_has_attribute(const struct sub* sub, uint64_t variable_id);
bool sub_has_attribute_str(struct config* config, const struct sub* sub, const char* attr);
bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir);

struct lnode* make_lnode(const struct config* config, struct cnode* parent);
void free_lnode(struct lnode* lnode);
struct cnode* make_cnode(const struct config* config, struct cdir* parent);
void free_cnode(struct cnode* cnode);

void fill_pred(struct sub* sub, const struct ast_node* expr);
struct sub* make_empty_sub(betree_sub_t id);
struct sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr);
struct event* make_event();
void event_to_string(const struct event* event, char* buffer);

struct pred* make_pred(const char* attr, betree_var_t variable_id, struct value value);
void add_pred(struct pred* pred, struct event* event);

void fill_event(const struct config* config, struct event* event);
bool validate_event(const struct config* config, const struct event* event);

struct event* make_event_from_string(const struct config* config, const char* event_str);

struct memoize make_memoize(size_t pred_count);
void free_memoize(struct memoize memoize);
void free_sub(struct sub* sub);

struct betree {
    struct config* config;
    struct cnode* cnode;
};

bool betree_delete_inner(struct config* config, struct sub* sub, struct cnode* cnode);
struct sub* find_sub_id(betree_sub_t id, struct cnode* cnode);

void betree_search_with_event(const struct config* config,
    const struct event* event,
    const struct cnode* cnode,
    struct report* report);

bool insert_be_tree(
    const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);

