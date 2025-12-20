#include "hmap.h"

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

// ENTRY FUNCTIONS

int init_Entry(struct Entry* entry){
    entry->key = newbuf(64);
    if (entry->key == NULL){
        return 0;
    }

    entry->val = newbuf(64);
    if (entry->val == NULL){
        freebuf(entry->key);
        return 0;
    }

    entry->heap_idx = (size_t)-1;
    return 1;
}

void free_Entry(struct Entry* entry){
    if (entry->key != NULL) freebuf(entry->key);
    if (entry->val != NULL) freebuf(entry->val);
}

int entry_eq(const char* key, struct HNode* node) {
    struct Entry *entry = container_of(node, struct Entry, node);
    return !bufstrcmp(entry->key, key, strlen(key));
}

uint64_t str_hash(const char *data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// HASH TABLE FUNCTIONS

static int init_HTab(struct HTab* htab, size_t cap){
    assert(cap > 0 && (cap & (cap - 1)) == 0);
    if (cap <= 0 || (cap & (cap - 1)) != 0){
        die("init_HTab (hmap.c): Invalid Capacity");
        return 0;
    }
    // checking if cap is a power of 2 or not (usefull as % operation is costly and since if cap is power of 2 then we can use & (cap - 1) instead of %)

    htab->tab = (struct HNode**)calloc(cap, sizeof(struct HNode*));
    if (htab->tab == NULL){
        msg("init_HTab (hmap.c): calloc");
        return 0;
    }
    htab->cap = cap;
    htab->size = 0;
    return 1;
}

static void insert_HTab(struct HTab* htab, struct HNode* node){
    size_t pos = node->hcode & (htab->cap - 1); // basically node->hcode % htab->cap
    node->next = htab->tab[pos];
    htab->tab[pos] = node;
    htab->size++;
}

static struct HNode** lookup_HTab(struct HTab* htab, const char* key, uint64_t hcode_key, int (*eq)(const char*, struct HNode*)){
    // returns pointer to the pointer of node for easy deletion
    if (htab == NULL || htab->tab == NULL){
        return NULL;
    }

    size_t pos = hcode_key & (htab->cap - 1);
    struct HNode** from = &htab->tab[pos];

    for (struct HNode* curr; (curr = *from) != NULL; from = &curr->next){
        if (curr->hcode == hcode_key && eq(key, curr)){
            return from;
        }
    }
    return NULL;
}

static struct HNode* delete_HTab(struct HTab* htab, const char* key, uint64_t hcode_key, int (*eq)(const char*, struct HNode*)){
    struct HNode** from = lookup_HTab(htab, key, hcode_key, eq);
    if (from == NULL){
        return NULL;
    }

    struct HNode* node = *from;
    *from = node->next;
    htab->size--;
    return node;
}

static struct HNode* delete_from_HTab(struct HTab* htab, struct HNode** from){
    if (from == NULL || *from == NULL){
        return NULL;
    }

    struct HNode* node = *from;
    *from = node->next;
    htab->size--;
    return node;
}

static void free_HTab(struct HTab* htab){
    if (htab != NULL) {
        free(htab->tab);
        htab->size = htab->cap = 0;
        htab->tab = NULL;
    }
}

// HASH MAP FUNCTIONS

int init_HMap(struct HMap* hmap, size_t cap){
    hmap->new_tab = malloc(sizeof(struct HTab));
    if (hmap->new_tab == NULL){
        msg("init_HMap (hmap.c): malloc");
        return 0;
    }

    hmap->old_tab = malloc(sizeof(struct HTab));
    if (hmap->old_tab == NULL){
        free(hmap->new_tab);
        msg("init_HMap (hmap.c): malloc");
        return 0;
    }

    hmap->old_tab->tab = hmap->new_tab->tab = NULL;
    hmap->old_tab->size = hmap->new_tab->size = 0;
    hmap->old_tab->cap = hmap->new_tab->cap = 0;

    if (init_HTab(hmap->new_tab, cap) == 0){
        free(hmap->new_tab);
        free(hmap->old_tab);
        return 0;
    }

    hmap->is_migrating = 0;
    hmap->migrating_pos = 0;
    return 1;
}

static int try_triggering_rehash(struct HMap* hmap){
    if (hmap->is_migrating == 1 || hmap->new_tab->size + 1 <= MAX_LOAD_FACTOR * hmap->new_tab->cap) return 1;

    struct HTab* tmp = hmap->old_tab;
    hmap->old_tab = hmap->new_tab;
    hmap->new_tab = tmp;
    if (init_HTab(hmap->new_tab, hmap->old_tab->cap * 2) == 0){
        return 0;
    }

    hmap->is_migrating = 1;
    hmap->migrating_pos = 0;
    return 1;
}

int insert_HMap(struct HMap* hmap, struct HNode* node){
    if (try_triggering_rehash(hmap) == 0){
        msg("insert_HMap (hmap.c): cannot trigger rehashing");
    }

    insert_HTab(hmap->new_tab, node);

    if (hmap->is_migrating == 1){
        if (hmap->migrating_pos == hmap->old_tab->cap){
            free_HTab(hmap->old_tab);
            hmap->is_migrating = 0;
        }
        else {
            int migration_count = 0;
            while (migration_count < MIGRATING_LOAD && hmap->old_tab->size > 0){
                struct HNode** from = &hmap->old_tab->tab[hmap->migrating_pos];

                if (*from == NULL){
                    hmap->migrating_pos++;
                }
                else {
                    insert_HTab(hmap->new_tab, delete_from_HTab(hmap->old_tab, from));
                    migration_count++;
                }
            }
        }
    }

    return 1;
}

struct HNode** lookup_HMap(struct HMap* hmap, const char* key, uint64_t hcode_key, int (*eq)(const char*, struct HNode*)){
    if (hmap == NULL || hmap->new_tab == NULL){
        return NULL;
    }

    struct HNode** from = lookup_HTab(hmap->new_tab, key, hcode_key, eq);
    if (from == NULL && hmap->is_migrating == 1){
        from = lookup_HTab(hmap->old_tab, key, hcode_key, eq);
    }

    return from;
}

struct HNode* delete_HMap(struct HMap* hmap, const char* key, uint64_t hcode_key, int (*eq)(const char*, struct HNode*)){
    if (hmap == NULL || hmap->new_tab == NULL){
        return NULL;
    }

    struct HNode* node = delete_HTab(hmap->new_tab, key, hcode_key, eq);
    if (node == NULL && hmap->is_migrating == 1){
        node = delete_HTab(hmap->old_tab, key, hcode_key, eq);
    }

    return node;
}

void free_HMap(struct HMap* hmap){
    if (hmap->new_tab != NULL){
        for (size_t i=0; i<hmap->new_tab->cap; ++i){
            if (hmap->new_tab->tab[i] == NULL) continue;
            struct HNode *curr = hmap->new_tab->tab[i];
            while (curr != NULL) {
                struct HNode *next = curr->next;
                struct Entry *entry = container_of(curr, struct Entry, node);
                free_Entry(entry);
                free(entry);
                curr = next;
            }
            hmap->new_tab->tab[i] = NULL;
        }
        free_HTab(hmap->new_tab);
        free(hmap->new_tab);
        hmap->new_tab = NULL;
    }
    if (hmap->old_tab != NULL){
        for (size_t i=0; i<hmap->old_tab->cap; ++i){
            if (hmap->old_tab->tab[i] == NULL) continue;
            struct HNode *curr = hmap->old_tab->tab[i];
            while (curr != NULL) {
                struct HNode *next = curr->next;
                struct Entry *entry = container_of(curr, struct Entry, node);
                free_Entry(entry);
                free(entry);
                curr = next;
            }
            hmap->old_tab->tab[i] = NULL;
        }
        free_HTab(hmap->old_tab);
        free(hmap->old_tab);
        hmap->old_tab = NULL;
    }
}