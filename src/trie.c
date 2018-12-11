#include <limits.h>

#include "alloc.h"
#include "trie.h"

struct trie* trie_new()
{
    struct trie* node = bmalloc(sizeof(*node));
	node->id = INVALID_STR;
    node->is_leaf = false;

	for (int i = 0; i < UCHAR_MAX; i++) {
		node->character[i] = NULL;
    }

	return node;
}

void trie_free(struct trie* trie)
{
    if(trie == NULL) {
        return;
    }
    for(size_t i = 0; i < UCHAR_MAX; i++) {
        if(trie->character[i] != NULL) {
            trie_free(trie->character[i]);
        }
    }
    bfree(trie);
}

void trie_insert(struct trie** head, const char* str, betree_str_t id)
{
	struct trie* curr = *head;
	while (*str) {
		if (curr->character[*str + 127] == NULL) {
			curr->character[*str + 127] = trie_new();
        }

		curr = curr->character[*str + 127];

		str++;
	}

	curr->id = id;
    curr->is_leaf = true;
}

betree_str_t trie_search(struct trie* head, const char* str)
{
	if (head == NULL)
		return INVALID_STR;

	struct trie* curr = head;
	while (*str) {
		curr = curr->character[*str + 127];

		if (curr == NULL) {
			return INVALID_STR;
        }

		str++;
	}

	return curr->id;
}

static bool have_children(struct trie* curr)
{
	for (int i = 0; i < UCHAR_MAX; i++) {
		if (curr->character[i]) {
			return true;
        }
    }
	return false;
}

bool trie_remove(struct trie** curr, const char* str)
{
	if (*curr == NULL) {
		return false;
    }

	if (*str) {
		if (*curr != NULL && (*curr)->character[*str + 127] != NULL && trie_remove(&((*curr)->character[*str + 127]), str + 1) && (*curr)->is_leaf == false) {
			if (!have_children(*curr)) {
				bfree(*curr);
				(*curr) = NULL;
				return true;
			}
			else {
				return false;
			}
		}
	}

	if (*str == '\0' && (*curr)->is_leaf) {
		if (!have_children(*curr)) {
			bfree(*curr);
			(*curr) = NULL;
			return true;
		}
		else {
			(*curr)->is_leaf = false;
			return false;
		}
	}

	return 0;
}

