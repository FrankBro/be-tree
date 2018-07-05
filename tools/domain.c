#include <string.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>

#include "ast.h"
#include "betree.h"
#include "tree.h"

struct value_bound get_integer_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .imin = INT64_MAX, .imax = INT64_MIN };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.ivalue < bound.imin) {
                    bound.imin = pred->value.ivalue;
                }
                if(pred->value.ivalue > bound.imax) {
                    bound.imax = pred->value.ivalue;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_integer_exprs_bound(const struct attr_domain* domain, struct ast_node** exprs, size_t expr_count) 
{
    struct value_bound bound = { .imin = INT64_MAX, .imax = INT64_MIN };
    for(size_t i = 0; i < expr_count; i++) {
        struct ast_node* expr = exprs[i];
        struct value_bound expr_bound = get_variable_bound(domain, expr);
        if(expr_bound.imin < bound.imin) {
            bound.imin = expr_bound.imin;
        }
        if(expr_bound.imax > bound.imax) {
            bound.imax = expr_bound.imax;
        }
    }
    return bound;
}

struct value_bound get_integer_list_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .is_integer_list_bounded = true, .ilmin = INT64_MAX, .ilmax = INT64_MIN };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                for(size_t k = 0; k < pred->value.ilvalue.count; k++) {
                    int64_t value = pred->value.ilvalue.integers[k];
                    if(value < bound.ilmin) {
                        bound.ilmin = value;
                    }
                    if(value > bound.ilmax) {
                        bound.ilmax = value;
                    }
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_integer_list_exprs_bound(const struct attr_domain* domain, struct ast_node** exprs, size_t expr_count) 
{
    struct value_bound bound = { .is_integer_list_bounded = true, .ilmin = INT64_MAX, .ilmax = INT64_MIN };
    for(size_t i = 0; i < expr_count; i++) {
        struct ast_node* expr = exprs[i];
        struct value_bound expr_bound = get_variable_bound(domain, expr);
        if(expr_bound.ilmin < bound.ilmin) {
            bound.ilmin = expr_bound.ilmin;
        }
        if(expr_bound.ilmax > bound.ilmax) {
            bound.ilmax = expr_bound.ilmax;
        }
    }
    return bound;
}

struct value_bound get_float_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .fmin = DBL_MAX, .fmax = -DBL_MAX };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.fvalue < bound.fmin) {
                    bound.fmin = pred->value.fvalue;
                }
                if(pred->value.fvalue > bound.fmax) {
                    bound.fmax = pred->value.fvalue;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_float_exprs_bound(const struct attr_domain* domain, struct ast_node** exprs, size_t expr_count) 
{
    struct value_bound bound = { .fmin = DBL_MAX, .fmax = -DBL_MAX };
    for(size_t i = 0; i < expr_count; i++) {
        struct ast_node* expr = exprs[i];
        struct value_bound expr_bound = get_variable_bound(domain, expr);
        if(expr_bound.fmin < bound.fmin) {
            bound.fmin = expr_bound.fmin;
        }
        if(expr_bound.fmax > bound.fmax) {
            bound.fmax = expr_bound.fmax;
        }
    }
    return bound;
}

struct value_bound get_string_events_bound(betree_var_t var, struct event** events, size_t event_count)
{
    struct value_bound bound = { .is_string_bounded = true, .smin = -1, .smax = 0 };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.svalue.str < bound.smin) {
                    bound.smin = pred->value.svalue.str;
                }
                if(pred->value.svalue.str > bound.smax && pred->value.svalue.str != UINT64_MAX) {
                    bound.smax = pred->value.svalue.str;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_string_exprs_bound(const struct attr_domain* domain, struct ast_node** exprs, size_t expr_count) 
{
    struct value_bound bound = { .is_string_bounded = true, .smin = -1, .smax = 0 };
    for(size_t i = 0; i < expr_count; i++) {
        struct ast_node* expr = exprs[i];
        struct value_bound expr_bound = get_variable_bound(domain, expr);
        if(expr_bound.smin < bound.smin) {
            bound.smin = expr_bound.smin;
        }
        if(expr_bound.smax > bound.smax) {
            bound.smax = expr_bound.smax;
        }
    }
    return bound;
}

struct value_bound get_string_list_events_bound(betree_var_t var, struct event** events, size_t event_count)
{
    struct value_bound bound = { .is_string_list_bounded = true, .slmin = -1, .slmax = 0 };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                for(size_t k = 0; k < pred->value.slvalue.count; k++) {
                    betree_str_t value = pred->value.slvalue.strings[k].str;
                    if(value < bound.smin) {
                        bound.slmin = value;
                    }
                    if(value > bound.smax && value != UINT64_MAX) {
                        bound.slmax = value;
                    }
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_string_list_exprs_bound(const struct attr_domain* domain, struct ast_node** exprs, size_t expr_count) 
{
    struct value_bound bound = { .is_string_list_bounded = true, .slmin = -1, .slmax = 0 };
    for(size_t i = 0; i < expr_count; i++) {
        struct ast_node* expr = exprs[i];
        struct value_bound expr_bound = get_variable_bound(domain, expr);
        if(expr_bound.slmin < bound.slmin) {
            bound.slmin = expr_bound.slmin;
        }
        if(expr_bound.slmax > bound.slmax) {
            bound.slmax = expr_bound.slmax;
        }
    }
    return bound;
}

struct attribute {
    betree_var_t var;
    size_t count;
};

struct operation {
    size_t attribute_count;
    struct attribute attributes[60]; // Can't be more than 60, for now
};

struct operations {
    struct operation set_in;
    struct operation set_not_in;
    struct operation list_one_of;
    struct operation list_none_of;
    struct operation list_all_of;
};

void add_attribute(betree_var_t var, struct operation* operation)
{
    for(size_t i = 0; i < operation->attribute_count; i++) {
        if(operation->attributes[i].var == var) {
            operation->attributes[i].count++;
            return;
        }
    }
    operation->attributes[operation->attribute_count].var = var;
    operation->attributes[operation->attribute_count].count = 1;
    operation->attribute_count++;
}

void extract_node(const struct ast_node* node, struct operations* operations)
{
    if(node->type == AST_TYPE_SET_EXPR) {
        if(node->set_expr.op == AST_SET_IN) {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                add_attribute(node->set_expr.left_value.variable_value.var, &operations->set_in);
            }
            else {
                add_attribute(node->set_expr.right_value.variable_value.var, &operations->set_in);
            }
        }
        if(node->set_expr.op == AST_SET_NOT_IN) {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                add_attribute(node->set_expr.left_value.variable_value.var, &operations->set_not_in);
            }
            else {
                add_attribute(node->set_expr.right_value.variable_value.var, &operations->set_not_in);
            }
        }
    }
    else if(node->type == AST_TYPE_LIST_EXPR) {
        if(node->list_expr.op == AST_LIST_ONE_OF) {
            add_attribute(node->list_expr.attr_var.var, &operations->list_one_of);
        }
        else if(node->list_expr.op == AST_LIST_NONE_OF) {
            add_attribute(node->list_expr.attr_var.var, &operations->list_none_of);
        }
        else {
            add_attribute(node->list_expr.attr_var.var, &operations->list_all_of);
        }
    }
    else if(node->type == AST_TYPE_BOOL_EXPR) {
        if(node->bool_expr.op == AST_BOOL_NOT) {
            extract_node(node->bool_expr.unary.expr, operations);
        }
        else if(node->bool_expr.op == AST_BOOL_OR || node->bool_expr.op == AST_BOOL_AND) {
            extract_node(node->bool_expr.binary.lhs, operations);
            extract_node(node->bool_expr.binary.rhs, operations);
        }
    }
}

void extract_cnode(struct cnode* cnode, struct operations* operations);

void extract_cdir(struct cdir* cdir, struct operations* operations)
{
    extract_cnode(cdir->cnode, operations);
    if(cdir->lchild != NULL) {
        extract_cdir(cdir->lchild, operations);
    }
    if(cdir->rchild != NULL) {
        extract_cdir(cdir->rchild, operations);
    }
}

void extract_cnode(struct cnode* cnode, struct operations* operations)
{
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        extract_node(cnode->lnode->subs[i]->expr, operations);
    }
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            extract_cdir(cnode->pdir->pnodes[i]->cdir, operations);
        }
    }
}

void print_operation(struct config* config, const char* name, struct operation* operation)
{
    printf("%s: [", name);
    for(size_t i = 0; i < operation->attribute_count; i++) {
        if(i != 0) {
            printf(", ");
        }
        const char* attr = config->attr_domains[operation->attributes[i].var]->attr_var.attr;
        printf("%s: %zu", attr, operation->attributes[i].count);
    }
    printf("]\n");
}

void extract_operations(struct betree* tree)
{
    struct operations operations;
    operations.set_in.attribute_count = 0;
    operations.set_not_in.attribute_count = 0;
    operations.list_one_of.attribute_count = 0;
    operations.list_none_of.attribute_count = 0;
    operations.list_all_of.attribute_count = 0;
    extract_cnode(tree->cnode, &operations);
    print_operation(tree->config, "set_in", &operations.set_in);
    print_operation(tree->config, "set_not_in", &operations.set_not_in);
    print_operation(tree->config, "list_one_of", &operations.list_one_of);
    print_operation(tree->config, "list_none_of", &operations.list_none_of);
    print_operation(tree->config, "list_all_of", &operations.list_all_of);
}

struct events {
    size_t count;
    char** events;
};

void add_event(char* event, struct events* events)
{
    if(events->count == 0) {
        events->events = calloc(1, sizeof(*events->events));
        if(events == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        char** new_events
            = realloc(events->events, sizeof(*new_events) * ((events->count) + 1));
        if(new_events == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        events->events = new_events;
    }
    events->events[events->count] = strdup(event);
    events->count++;
}

char* strip_chars(const char* string, const char* chars)
{
    char* new_string = malloc(strlen(string) + 1);
    int counter = 0;

    for(; *string; string++) {
        if(!strchr(chars, *string)) {
            new_string[counter] = *string;
            ++counter;
        }
    }
    new_string[counter] = 0;
    return new_string;
}

int event_parse(const char* text, struct event** event);

#define MAX_EXPRS 5000
#define MAX_EVENTS 1000

size_t read_betree_events(struct config* config, struct events* events)
{
    FILE* f = fopen("betree_events", "r");
    size_t count = 0;

    char line[22000]; // Arbitrary from what I've seen
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EVENTS != 0 && events->count == MAX_EVENTS) {
            break;
        }

        struct event* event;
        if(event_parse(line, &event) != 0) {
            fprintf(stderr, "Can't parse event %zu: %s", events->count + 1, line);
            abort();
        }

        fill_event(config, event);
        if(!validate_event(config, event)) {
            abort();
        }
        add_event(line, events);
        count++;
    }
    fclose(f);
    return count;
}

size_t read_betree_exprs(struct betree* tree)
{
    FILE* f = fopen("betree_exprs", "r");

    //char* lines[MAX_EXPRS];
    char line[10000]; // Arbitrary from what I've seen
    size_t count = 0;
    while(fgets(line, sizeof(line), f)) {
        if(!betree_insert(tree, count, line)) {
            printf("Can't insert expr: %s\n", line);
            abort();
        }
        count++;
        if(MAX_EXPRS != 0 && count == MAX_EXPRS) {
            break;
        }
    }

    /*
    while(fgets(line, sizeof(line), f)) {
        lines[count] = strdup(line);
        count++;
        if(MAX_EXPRS != 0 && count == MAX_EXPRS) {
            break;
        }
    }

    betree_insert_all(tree, count, (const char**)lines);
    */

    fclose(f);
    return count;
}

void read_betree_defs(struct betree* tree)
{
    FILE* f = fopen("betree_defs", "r");

    char line[LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        betree_add_domain(tree, line);
    }

    fclose(f);
}

size_t extract_exprs_cnode(struct cnode* cnode, struct ast_node** exprs, size_t count);

size_t extract_exprs_cdir(struct cdir* cdir, struct ast_node** exprs, size_t count)
{
    count = extract_exprs_cnode(cdir->cnode, exprs, count);
    if(cdir->lchild != NULL) {
        count = extract_exprs_cdir(cdir->lchild, exprs, count);
    }
    if(cdir->rchild != NULL) {
        count = extract_exprs_cdir(cdir->rchild, exprs, count);
    }
    return count;
}

size_t extract_exprs_cnode(struct cnode* cnode, struct ast_node** exprs, size_t count)
{
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        exprs[count + i] = (struct ast_node*)cnode->lnode->subs[i]->expr;
    }
    count += cnode->lnode->sub_count;
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            count = extract_exprs_cdir(cnode->pdir->pnodes[i]->cdir, exprs,  count);
        }
    }
    return count;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    struct betree* tree = betree_make();
    read_betree_defs(tree);
    
    size_t expr_count = read_betree_exprs(tree);

    extract_operations(tree);

    struct events events = { .count = 0, .events = NULL };
    size_t event_count = read_betree_events(tree->config, &events);

    struct event* parsed_events[event_count];
    for(size_t i = 0; i < event_count; i++) {
        parsed_events[i] = make_event_from_string(tree->config, events.events[i]);
    }

    struct ast_node* parsed_exprs[expr_count];
    extract_exprs_cnode(tree->cnode, parsed_exprs, 0);

    for(size_t i = 0; i < tree->config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = tree->config->attr_domains[i];
        if(attr_domain->bound.value_type == VALUE_I) {
            struct value_bound events_bound = get_integer_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            struct value_bound exprs_bound = get_integer_exprs_bound(attr_domain, parsed_exprs, expr_count);
            printf("    i, %s: expr: [%ld, %ld], event: [%ld, %ld]\n", attr_domain->attr_var.attr, exprs_bound.imin, exprs_bound.imax, events_bound.imin, events_bound.imax);
        }
        else if(attr_domain->bound.value_type == VALUE_F) {
            struct value_bound events_bound = get_float_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            struct value_bound exprs_bound = get_float_exprs_bound(attr_domain, parsed_exprs, expr_count);
            printf("    f, %s: expr: [%.2f, %.2f], event: [%.2f, %.2f]\n", attr_domain->attr_var.attr, exprs_bound.fmin, exprs_bound.fmax, events_bound.fmin, events_bound.fmax);
        }
        else if(attr_domain->bound.value_type == VALUE_S) {
            struct value_bound events_bound = get_string_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            struct value_bound exprs_bound = get_string_exprs_bound(attr_domain, parsed_exprs, expr_count);
            printf("    s, %s: expr: %zu values, event: %zu values\n", attr_domain->attr_var.attr, exprs_bound.smax, events_bound.smax);
        }
        else if(attr_domain->bound.value_type == VALUE_IL) {
            struct value_bound events_bound = get_integer_list_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            struct value_bound exprs_bound = get_integer_list_exprs_bound(attr_domain, parsed_exprs, expr_count);
            printf("    il, %s: expr: [%ld, %ld], event: [%ld, %ld]\n", attr_domain->attr_var.attr, exprs_bound.ilmin, exprs_bound.ilmax, events_bound.ilmin, events_bound.ilmax);
        }
        else if(attr_domain->bound.value_type == VALUE_SL) {
            struct value_bound events_bound = get_string_list_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            struct value_bound exprs_bound = get_string_list_exprs_bound(attr_domain, parsed_exprs, expr_count);
            printf("    sl, %s: expr: %zu values, event: %zu values\n", attr_domain->attr_var.attr, exprs_bound.slmax, events_bound.slmax);
        }
    }

    return 0;
}

