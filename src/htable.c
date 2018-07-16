/* htable.c
   Rémi Attab (remi.attab@gmail.com), 10 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/
#include <stdlib.h>

#include "htable.h"


// -----------------------------------------------------------------------------
// hash
// -----------------------------------------------------------------------------

// FNV-1a hash implementation: http://isthe.com/chongo/tech/comp/fnv/
static inline uint64_t htable_hash(const char *key)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < strnlen(key, htable_key_max_len); ++i)
        hash = (hash ^ key[i]) * 0x100000001b3;

    return hash;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum { probe_window = 8 };


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void htable_reset(struct htable *ht)
{
    if (ht->table) {
        for (size_t i = 0; i < ht->cap; ++i) {
            if (ht->table[i].key) free((char *) ht->table[i].key);
        }
        free(ht->table);
    }
    *ht = (struct htable) {0};
}

static bool table_put(
        struct htable_bucket *table, size_t cap,
        const char *key, struct ast_node* value)
{
    uint64_t hash = htable_hash(key);

    for (size_t i = 0; i < probe_window; ++i) {
        struct htable_bucket *bucket = &table[(hash + i) % cap];
        if (bucket->key) continue;

        bucket->key = key;
        bucket->value = value;
        return true;
    }

    return false;
}

static void htable_resize(struct htable *ht, size_t cap)
{
    if (cap <= ht->cap) return;

    size_t new_cap = ht->cap ? ht->cap : 1;
    while (new_cap < cap) new_cap *= 2;

    struct htable_bucket *new_table = calloc(new_cap, sizeof(*new_table));

    for (size_t i = 0; i < ht->cap; ++i) {
        struct htable_bucket *bucket = &ht->table[i];
        if (!bucket->key) continue;

        if (!table_put(new_table, new_cap, bucket->key, bucket->value)) {
            free(new_table);
            htable_resize(ht, new_cap * 2);
            return;
        }
    }

    free(ht->table);
    ht->cap = new_cap;
    ht->table = new_table;
}

void htable_reserve(struct htable *ht, size_t items)
{
    htable_resize(ht, items * 4);
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

struct htable_ret htable_get(struct htable *ht, const char *key)
{
    uint64_t hash = htable_hash(key);
    htable_resize(ht, probe_window);

    for (size_t i = 0; i < probe_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (!bucket->key) continue;
        if (strncmp(bucket->key, key, htable_key_max_len)) continue;

        return (struct htable_ret) { .ok = true, .value = bucket->value };
    }

    return (struct htable_ret) { .ok = false };
}

struct htable_ret htable_put(struct htable *ht, const char *key, struct ast_node* value)
{
    uint64_t hash = htable_hash(key);
    htable_resize(ht, probe_window);

    struct htable_bucket *empty = NULL;

    for (size_t i = 0; i < probe_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (bucket->key) {
            if (strncmp(bucket->key, key, htable_key_max_len)) continue;
            return (struct htable_ret) { .ok = false, .value = bucket->value };
        }

        if (!empty) empty = bucket;
    }

    if (empty) {
        ht->len++;
        empty->key = strndup(key, htable_key_max_len);
        empty->value = value;
        return (struct htable_ret) { .ok = true };
    }

    htable_resize(ht, ht->cap * 2);
    return htable_put(ht, key, value);
}

struct htable_ret htable_xchg(struct htable *ht, const char *key, struct ast_node* value)
{
    uint64_t hash = htable_hash(key);
    htable_resize(ht, probe_window);

    for (size_t i = 0; i < probe_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (!bucket->key) continue;
        if (strncmp(bucket->key, key, htable_key_max_len)) continue;

        struct ast_node* old_value = bucket->value;
        bucket->value = value;
        return (struct htable_ret) { .ok = true, .value = old_value };
    }

    return (struct htable_ret) { .ok = false };
}

struct htable_ret htable_del(struct htable *ht, const char *key)
{
    uint64_t hash = htable_hash(key);
    htable_resize(ht, probe_window);

    for (size_t i = 0; i < probe_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (!bucket->key) continue;
        if (strncmp(bucket->key, key, htable_key_max_len)) continue;

        ht->len--;
        free((char *) bucket->key);
        bucket->key = NULL;
        return (struct htable_ret) { .ok = true, .value = bucket->value };
    }

    return (struct htable_ret) { .ok = false };
}


struct htable_bucket * htable_next(
        struct htable *ht, struct htable_bucket *bucket)
{
    if (!ht->table) return NULL;

    size_t i = 0;
    if (bucket) i = (bucket - ht->table) + 1;

    for (; i < ht->cap; ++i) {
        bucket = &ht->table[i];
        if (bucket->key) return bucket;
    }

    return NULL;
}

void htable_diff(struct htable *a, struct htable *b, struct htable *result)
{
    struct htable_bucket *it;
    for (it = htable_next(a, NULL); it; it = htable_next(a, it)) {
        if (htable_get(b, it->key).ok) continue;
        htable_put(result, it->key, it->value);
    }
}

