#pragma once

#include <stdbool.h>

#include "ast.h"
#include "tree.h"

char* ast_to_string(const struct ast_node* node);
void print_variable(const struct betree_variable* v);
void print_attr_domain(const struct attr_domain* domain);
void print_cdir(const struct cdir* cdir);
void print_report(const struct report* report);
void print_value_type(enum betree_value_type_e value_type);

