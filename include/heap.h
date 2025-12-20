#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "logging_helper.h"
#include "config.h"

struct HeapItem {
    uint64_t expiration_time;
    size_t* ref;
};

struct Heap {
    struct HeapItem* arr;
    size_t sz;
    size_t cap;
};

void heap_update(struct HeapItem* arr, size_t pos, size_t len);
int heap_init(struct Heap* heap, size_t cap);
void heap_free(struct Heap* heap);
int heap_insert(struct Heap* heap, uint64_t expiration_time, size_t* ref);
void heap_delete(struct Heap* heap, size_t pos);

#endif