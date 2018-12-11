#pragma once

#include <limits.h>

#include "value.h"

struct trie {
    betree_str_t id;
    struct trie* character[UCHAR_MAX];
    bool is_leaf;
};

struct trie* trie_new();
void trie_free(struct trie* trie);
void trie_insert(struct trie** head, const char* str, betree_str_t id);
betree_str_t trie_search(struct trie* head, const char* str);
bool trie_remove(struct trie** curr, const char* str);

