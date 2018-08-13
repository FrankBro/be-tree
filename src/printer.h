#pragma once

#include <stdbool.h>

#include "ast.h"

char* ast_to_string(const struct ast_node* node);
void print_variable(const struct betree_variable* v);

