#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint64_t betree_var_t;
typedef uint64_t betree_sub_t;
typedef uint64_t betree_str_t;
typedef uint64_t betree_seg_t;

enum value_e {
    VALUE_B,
    VALUE_I,
    VALUE_F,
    VALUE_S,
    VALUE_IL,
    VALUE_SL,
    VALUE_SEGMENTS,
};

struct string_value {
    const char* string;
    betree_str_t str;
};

struct integer_list_value {
    size_t count;
    int64_t* integers;
};

struct string_list_value {
    size_t count;
    struct string_value* strings;
};

struct segment {
    int64_t id;
    int64_t timestamp;
};

struct segments_list {
    size_t size;
    struct segment* content;
};

struct value {
    enum value_e value_type;
    union {
        int64_t ivalue;
        double fvalue;
        bool bvalue;
        struct string_value svalue;
        struct integer_list_value ilvalue;
        struct string_list_value slvalue;
        struct segments_list segments_value;
    };
};

struct value_bound {
    enum value_e value_type;
    union {
        struct {
            int64_t imin;
            int64_t imax;
        };
        struct {
            double fmin;
            double fmax;
        };
        struct {
            bool bmin;
            bool bmax;
        };
    };
};

struct pred {
    betree_var_t variable_id;
    struct value value;
};

struct sub {
    betree_sub_t id;
    size_t variable_id_count;
    betree_var_t* variable_ids;
    const struct ast_node *expr;
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
    struct sub **subs;
    size_t max;
};

struct pdir;
struct pnode;
struct cdir;

struct cnode {
    struct cdir* parent;
    struct lnode *lnode;
    struct pdir *pdir;
};

struct cdir;
struct pdir;

struct pnode {
    struct pdir* parent;
    betree_var_t variable_id;
    struct cdir *cdir;
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
    betree_var_t variable_id;
    struct value_bound bound;
    struct cnode *cnode;
    struct cdir *lchild;
    struct cdir *rchild;
};

struct pdir {
    struct cnode* parent;
    size_t pnode_count;
    struct pnode** pnodes;
};

struct attr_domain {
    betree_var_t variable_id;
    struct value_bound bound;
    bool allow_undefined;
};

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
};

struct config* make_config(uint64_t lnode_max_cap, uint64_t partition_min_size);
struct config* make_default_config();
void free_config(struct config* config);
void add_attr_domain(struct config* config, const char* attr, struct value_bound bound, bool allow_undefined);
void add_attr_domain_i(struct config* config, const char* attr, int64_t min, int64_t max, bool allow_undefined);
void add_attr_domain_f(struct config* config, const char* attr, double min, double max, bool allow_undefined);
void add_attr_domain_b(struct config* config, const char* attr, bool min, bool max, bool allow_undefined);
void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
void adjust_attr_domains(struct config* config, const struct ast_node* node, struct value_bound bound, bool allow_undefined);
void adjust_attr_domains_i(struct config* config, const struct ast_node* node, int64_t min, int64_t max, bool allow_undefined);
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
    betree_sub_t *subs;
};

struct matched_subs* make_matched_subs();
void free_matched_subs(struct matched_subs* matched_subs);
const struct pred* make_simple_pred_i(betree_var_t variable_id, int64_t value);
const struct pred* make_simple_pred_f(betree_var_t variable_id, double fvalue);
const struct pred* make_simple_pred_segment(betree_var_t variable_id, int64_t id, int64_t timestamp);
const struct pred* make_simple_pred_str_i(struct config* config, const char* attr, int64_t value);
const struct pred* make_simple_pred_str_il(struct config* config, const char* attr, struct integer_list_value value);
const struct pred* make_simple_pred_str_sl(struct config* config, const char* attr, struct string_list_value value);
void fill_pred(struct sub* sub, const struct ast_node* expr);
struct sub* make_empty_sub(betree_sub_t id);
const struct sub* make_sub(struct config* config, betree_sub_t id, struct ast_node* expr);
const struct event* make_event();
const struct event* make_simple_event_i(struct config* config, const char* attr, int64_t value);
const struct event* make_simple_event_s(struct config* config, const char* attr, const char* value);
const struct event* make_simple_event_il(struct config* config, const char* attr, struct integer_list_value value);
const struct event* make_simple_event_sl(struct config* config, const char* attr, struct string_list_value value);
void event_to_string(struct config* config, const struct event* event, char* buffer);

void insert_be_tree(const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);
void match_be_tree(struct config* config, const struct event* event, const struct cnode* cnode, struct matched_subs* matched_subs);
bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode);

void add_integer_list_value(int64_t integer, struct integer_list_value* list);
const char* integer_list_value_to_string(struct integer_list_value list);
void add_string_list_value(struct string_value string, struct string_list_value* list);
const char* string_list_value_to_string(struct string_list_value list);
