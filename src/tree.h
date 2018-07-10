#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "betree.h"
#include "memoize.h"
#include "value.h"

typedef uint64_t betree_seg_t;

struct attr_var {
    const char* attr;
    betree_var_t var;
};

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
    size_t attr_var_count;
    struct attr_var* attr_vars;
    const struct ast_node* expr;
    struct short_circuit short_circuit;
};

struct event {
    size_t pred_count;
    // TODO Make const
    struct pred** preds;
};

struct cnode;

struct lnode {
    struct cnode* parent;
    size_t sub_count;
    // TODO Make const
    struct sub** subs;
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
    size_t pnode_count;
    struct pnode** pnodes;
};

struct attr_domain {
    struct attr_var attr_var;
    struct value_bound bound;
    bool allow_undefined;
};

struct ast_node;
struct pred_map;

struct string_map {
    struct attr_var attr_var;
    size_t string_value_count;
    char** string_values;
};

struct config {
    uint64_t lnode_max_cap;
    uint64_t partition_min_size;
    uint64_t max_domain_for_split;
    bool abort_on_error;
    size_t attr_domain_count;
    struct attr_domain** attr_domains;
    size_t attr_to_id_count;
    // TODO Make const
    size_t string_map_count;
    struct string_map* string_maps;
    char** attr_to_ids;
    struct pred_map* pred_map;
};

struct config* make_config(uint64_t lnode_max_cap, uint64_t partition_min_size);
struct config* make_default_config();
void free_config(struct config* config);

void add_attr_domain(struct config* config, const char* attr, struct value_bound bound, bool allow_undefined);
void add_attr_domain_i(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_f(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_b(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined);

void add_attr_domain_bounded_i(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max);
void add_attr_domain_bounded_f(struct config* config, const char* attr, bool allow_undefined, double min, double max);
void add_attr_domain_bounded_s(struct config* config, const char* attr, bool allow_undefined, size_t max);
void add_attr_domain_bounded_il(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max);
void add_attr_domain_bounded_sl(struct config* config, const char* attr, bool allow_undefined, size_t max);
const struct attr_domain* get_attr_domain(const struct config* config, betree_var_t variable_id);
bool is_variable_allow_undefined(const struct config* config, const betree_var_t variable_id);

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id);
betree_var_t try_get_id_for_attr(const struct config* config, const char* attr);
betree_var_t get_id_for_attr(struct config* config, const char* attr);
betree_str_t try_get_id_for_string(const struct config* config, struct attr_var attr_var, const char* string);
betree_str_t get_id_for_string(struct config* config, struct attr_var attr_var, const char* string);

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

struct attr_var make_attr_var(const char* attr, struct config* config);
struct attr_var copy_attr_var(struct attr_var attr_var);
void free_attr_var(struct attr_var attr_var);

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
    struct event* event,
    const struct cnode* cnode,
    struct report* report);

bool insert_be_tree(
    const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);
