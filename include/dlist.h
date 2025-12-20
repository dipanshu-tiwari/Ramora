#ifndef DLIST_H
#define DLIST_H

struct DList {
    struct DList* prev;
    struct DList* next;
};

static inline void dlist_init(struct DList* node){
    node->prev = node->next = node;
}

static inline int dlist_empty(struct DList* node){
    if (node->next == node) return 1;
    else return 0;
}

static inline void dlist_insert_before(struct DList* target, struct DList* node){
    struct DList* prev = target->prev;
    prev->next = node;
    node->prev = prev;
    node->next = target;
    target->prev = node;
}

static inline void dlist_delete(struct DList* node){
    struct DList* prev = node->prev;
    struct DList* next = node->next;
    prev->next = next;
    next->prev = prev;
}

#endif