#pragma once
#include "list.h"
#include <stddef.h>

struct hash {
    size_t bucket_count;
    struct list_head *buckets;
};

int hash_init(struct hash *hs, size_t n);
int hash_insert(struct hash *hs, const char *key, void *value);
int hash_lookup(struct hash *hs, const char *key, void **value);
