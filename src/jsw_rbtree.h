#pragma once

#include <stddef.h>

struct jsw_rbtree;

typedef int (*cmp_f) (const void *p1, const void *p2);

struct jsw_rbtree* jsw_rbnew (cmp_f cmp);
void jsw_rbdelete(struct jsw_rbtree* tree);
void* jsw_rbfind(struct jsw_rbtree* tree, void* data);
int jsw_rbinsert(struct jsw_rbtree* tree, void* data);
int jsw_rberase(struct jsw_rbtree* tree, void* data);
size_t jsw_rbsize(struct jsw_rbtree* tree);

