#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "memoize.h"
#include "value.h"

typedef uint64_t betree_var_t;
typedef uint64_t betree_sub_t;
typedef uint64_t betree_seg_t;

struct attr_var {
    const char* attr;
    betree_var_t var;
};

struct pred {
    struct attr_var attr_var;
    struct value value;
};

struct sub {
    betree_sub_t id;
    size_t attr_var_count;
    struct attr_var* attr_vars;
    const struct ast_node* expr;
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

struct config {
    uint64_t lnode_max_cap;
    uint64_t partition_min_size;
    size_t attr_domain_count;
    struct attr_domain** attr_domains;
    size_t attr_to_id_count;
    // TODO Make const
    char** attr_to_ids;
    size_t string_value_count;
    char** string_values;
    betree_pred_t pred_count;
    struct ast_node** preds;
};

struct config* make_config(uint64_t lnode_max_cap, uint64_t partition_min_size);
struct config* make_default_config();
void free_config(struct config* config);
void add_attr_domain(
    struct config* config, const char* attr, struct value_bound bound, bool allow_undefined);
void add_attr_domain_i(
    struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined);
void add_attr_domain_f(
    struct config* config, const char* attr, double min, double max, bool allow_undefined);
void add_attr_domain_b(
    struct config* config, const char* attr, bool min, bool max, bool allow_undefined);
void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined);
void adjust_attr_domains(struct config* config,
    const struct ast_node* node,
    struct value_bound bound,
    bool allow_undefined);
void adjust_attr_domains_i(struct config* config,
    const struct ast_node* node,
    int64_t min,
    int64_t max,
    bool allow_undefined);
const struct attr_domain* get_attr_domain(const struct config* config, betree_var_t variable_id);
bool is_variable_allow_undefined(const struct config* config, const betree_var_t variable_id);

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id);
betree_var_t get_id_for_attr(struct config* config, const char* attr);
betree_str_t get_id_for_string(struct config* config, const char* string);

void free_sub(struct sub* sub);
void free_event(struct event* event);

bool sub_has_attribute(const struct sub* sub, uint64_t variable_id);
bool sub_has_attribute_str(struct config* config, const struct sub* sub, const char* attr);
bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir);

void insert_sub(const struct sub* sub, struct lnode* lnode);
bool remove_sub(const struct sub* sub, struct lnode* lnode);

struct lnode* make_lnode(const struct config* config, struct cnode* parent);
void free_lnode(struct lnode* lnode);
struct cnode* make_cnode(const struct config* config, struct cdir* parent);
void free_cnode(struct cnode* cnode);

struct matched_subs {
    size_t sub_count;
    betree_sub_t* subs;
};

struct report {
    size_t expressions_evaluated;
    size_t expressions_matched;
    size_t expressions_memoized;
    size_t sub_expressions_memoized;
};

struct matched_subs* make_matched_subs();
void free_matched_subs(struct matched_subs* matched_subs);
const struct pred* make_simple_pred_b(const char* attr, betree_var_t variable_id, bool bvalue);
const struct pred* make_simple_pred_i(const char* attr, betree_var_t variable_id, int64_t value);
const struct pred* make_simple_pred_f(const char* attr, betree_var_t variable_id, double fvalue);
const struct pred* make_simple_pred_s(
    struct config* config, betree_var_t variable_id, const char* svalue);
const struct pred* make_simple_pred_segment(
    const char* attr, betree_var_t variable_id, int64_t id, int64_t timestamp);
const struct pred* make_simple_pred_frequency(betree_var_t variable_id,
    enum frequency_type_e type,
    uint32_t id,
    struct string_value ns,
    bool timestamp_defined,
    int64_t timestamp,
    uint32_t cap_value);
const struct pred* make_simple_pred_str_i(struct config* config, const char* attr, int64_t value);
const struct pred* make_simple_pred_str_il(
    struct config* config, const char* attr, struct integer_list_value value);
const struct pred* make_simple_pred_str_sl(
    struct config* config, const char* attr, struct string_list_value value);
void fill_pred(struct sub* sub, const struct ast_node* expr);
struct sub* make_empty_sub(betree_sub_t id);
const struct sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr);
struct event* make_event();
const struct event* make_simple_event_i(struct config* config, const char* attr, int64_t value);
const struct event* make_simple_event_s(struct config* config, const char* attr, const char* value);
const struct event* make_simple_event_il(
    struct config* config, const char* attr, struct integer_list_value value);
const struct event* make_simple_event_sl(
    struct config* config, const char* attr, struct string_list_value value);
void event_to_string(const struct event* event, char* buffer);

void insert_be_tree(
    const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);
void match_be_tree(struct config* config,
    const struct event* event,
    const struct cnode* cnode,
    struct matched_subs* matched_subs, struct report* report, struct memoize* memoize);
bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode);

void betree_insert(struct config* config, betree_sub_t id, const char* expr, struct cnode* cnode);
void betree_search_with_event(struct config* config,
    struct event* event,
    const struct cnode* cnode,
    struct matched_subs* matched_subs,
    struct report* report);
void betree_search(struct config* config,
    const char* event,
    const struct cnode* cnode,
    struct matched_subs* matched_subs,
    struct report* report);

struct attr_var make_attr_var(const char* attr, struct config* config);
struct attr_var copy_attr_var(struct attr_var attr_var);
void free_attr_var(struct attr_var attr_var);

struct pred* make_pred(const char* attr, betree_var_t variable_id, struct value value);
void add_pred(struct pred* pred, struct event* event);

void fill_event(struct config* config, struct event* event);
bool validate_event(const struct config* config, const struct event* event);

struct report make_empty_report();
