#include "hash.h"
#include <stdlib.h>
#include <string.h>

struct hash_entry {
    char key[64];
    void *value;
    struct list_head link;
};

static size_t strhash(const char *str) {
    size_t r = 5381;
    int c;

    while ((c = *str++)) {
        r = ((r << 5) + r) + c;
    }

    return r;
}

int hash_init(struct hash *hs, size_t cap) {
    hs->bucket_count = cap;
    hs->buckets = malloc(sizeof(struct list_head) * cap);
    if (!hs->buckets) {
        return -1;
    }
    for (size_t i = 0; i < cap; ++i) {
        list_head_init(&hs->buckets[i]);
    }
    return 0;
}

int hash_lookup(struct hash *hs, const char *key, void **value) {
    size_t hash = strhash(key) % hs->bucket_count;
    struct hash_entry *ent;

    list_for_each_entry(ent, &hs->buckets[hash], link) {
        if (!strcmp(ent->key, key)) {
            if (value) {
                *value = ent->value;
            }
            return 0;
        }
    }

    return -1;
}

int hash_insert(struct hash *hs, const char *key, void *value) {
    // TODO: check if already present
    size_t hash = strhash(key) % hs->bucket_count;
    struct hash_entry *ent = malloc(sizeof(struct hash_entry));
    if (!ent) {
        return -1;
    }

    list_head_init(&ent->link);
    strcpy(ent->key, key);
    ent->value = value;

    list_add(&ent->link, &hs->buckets[hash]);
    return 0;
}
