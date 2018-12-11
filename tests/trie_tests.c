#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minunit.h"
#include "trie.h"

int test_trie()
{
    struct trie* trie = trie_new();
    betree_str_t next_id = 0;

    printf("DEBUG: %zu\n", trie_search(trie, "a"));
    mu_assert(trie_search(trie, "a") == INVALID_STR, "Empty");

    trie_insert(&trie, "a", next_id);
    next_id++;
    mu_assert(trie_search(trie, "a") == 0, "Found");

    trie_remove(&trie, "a");
    mu_assert(trie_search(trie, "a") == INVALID_STR, "Empty");

    trie_free(trie);
    return 0;
}

int all_tests()
{
    mu_run_test(test_trie);

    return 0;
}

RUN_TESTS()

