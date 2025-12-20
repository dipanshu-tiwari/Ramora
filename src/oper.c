#include "oper.h"

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

int set_ttl_Entry_ms(char* key, int64_t ttl){
    struct HMap* hmap = &gd.kv_db;

    uint64_t hcode_key = str_hash(key, strlen(key));
    struct HNode** from = lookup_HMap(hmap, key, hcode_key, entry_eq);

    if (from == NULL || *from == NULL){
        return 0;
    }

    struct Entry* entry = (struct Entry*)container_of(*from, struct Entry, node);
    if (ttl == -1){
        if (entry->heap_idx != (size_t)-1){
            heap_delete(&gd.ttl_heap, entry->heap_idx);
            entry->heap_idx = (size_t)-1;
        }
        return 1;
    }

    uint64_t expiration_time = get_monotonic_msec() + (uint64_t)ttl;

    if (entry->heap_idx == (size_t)-1){
        if (heap_insert(&gd.ttl_heap, expiration_time, &entry->heap_idx) == 0){
            return -1;
        }
    }
    else {
        gd.ttl_heap.arr[entry->heap_idx].expiration_time = expiration_time;
        heap_update(gd.ttl_heap.arr, entry->heap_idx, gd.ttl_heap.sz);
    }
    return 1;
}

int64_t get_ttl_Entry_ms(char* key){
    struct HMap* hmap = &gd.kv_db;

    uint64_t hcode_key = str_hash(key, strlen(key));
    struct HNode** from = lookup_HMap(hmap, key, hcode_key, entry_eq);

    if (from == NULL || *from == NULL){
        return -2; // -2 for RES_NX -> key not present
    }

    struct Entry* entry = (struct Entry*)container_of(*from, struct Entry, node);
    if (entry->heap_idx == (size_t)-1){
        return -1; // -1 for no ttl -> not set for expiration
    }

    int64_t ttl = gd.ttl_heap.arr[entry->heap_idx].expiration_time - get_monotonic_msec();
    if (ttl < 0) ttl = 0;
    return ttl;
}

int set_ttl_Entry_sec(char* key, int64_t ttl){
    if (ttl != -1) ttl *= 1000;
    return set_ttl_Entry_ms(key, ttl);
}

int64_t get_ttl_Entry_sec(char* key){
    int64_t ttl = get_ttl_Entry_ms(key);
    if (ttl < 0) return ttl;
    else return ttl / 1000;
}

struct buf* get_Entry(char* key){
    struct HMap* hmap = &gd.kv_db;

    uint64_t hcode_key = str_hash(key, strlen(key));
    struct HNode** from = lookup_HMap(hmap, key, hcode_key, entry_eq);

    if (from == NULL || *from == NULL){
        return NULL;
    }

    return container_of(*from, struct Entry, node)->val;
}

int set_Entry(char* key, char* val){
    struct HMap* hmap = &gd.kv_db;

    size_t key_len = strlen(key);
    size_t val_len = strlen(val);
    uint64_t hcode_key = str_hash(key, key_len);
    struct HNode** from = lookup_HMap(hmap, key, hcode_key, entry_eq);

    int rv = 1;
    if (from != NULL && *from != NULL){
        struct Entry* found_entry = container_of(*from, struct Entry, node);
        clearbuf(found_entry->val);
        rv &= bufappend(found_entry->val, val, val_len);
        return rv;
    }

    struct Entry* entry = malloc(sizeof(struct Entry));
    if (entry == NULL){
        msg("set_Entry (oper.c): malloc");
        return 0;
    }
    if (init_Entry(entry) == 0){
        free(entry);
        return 0;
    }

    rv &= bufappend(entry->key, key, key_len + 1);
    rv &= bufappend(entry->val, val, val_len + 1);
    entry->node.hcode = hcode_key;
    rv &= insert_HMap(hmap, &entry->node);
    return rv;
}

int del_Entry(char* key){
    struct HMap* hmap = &gd.kv_db;

    uint64_t hcode_key = str_hash(key, strlen(key));
    struct HNode* node = delete_HMap(hmap, key, hcode_key, entry_eq);

    if (node == NULL){
        return 0;
    }
    else {
        struct Entry* entry = container_of(node, struct Entry, node);
        if (entry->heap_idx != (size_t)-1) heap_delete(&gd.ttl_heap, entry->heap_idx);
        free_Entry(entry);
        free(entry);
        return 1;
    }
}