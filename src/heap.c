#include "heap.h"
#include <stdio.h>

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

static size_t heap_left(size_t pos){
    return 2 * pos + 1;
}

static size_t heap_right(size_t pos){
    return 2 * pos + 2;
}

static size_t heap_parent(size_t pos){
    return (pos - 1) / 2;
}

static void heap_up(struct HeapItem* arr, size_t pos){
    struct HeapItem element = arr[pos];
    while (pos > 0 && arr[heap_parent(pos)].expiration_time > arr[pos].expiration_time){
        arr[pos] = arr[heap_parent(pos)];
        *arr[pos].ref = pos;
        pos = heap_parent(pos);
    }
    arr[pos] = element;
    *arr[pos].ref = pos;
}

static void heap_down(struct HeapItem* arr, size_t pos, size_t len){
    struct HeapItem element = arr[pos];
    while(1){
        size_t min_element = pos;
        size_t left = heap_left(pos);
        size_t right = heap_right(pos);
        if (left < len && arr[left].expiration_time < arr[min_element].expiration_time) min_element = left;
        if (right < len && arr[right].expiration_time < arr[min_element].expiration_time) min_element = right;

        if (min_element == pos) break;
        else {
            arr[pos] = arr[min_element];
            *arr[pos].ref = pos;
            pos = min_element;
        }
    }
    arr[pos] = element;
    *arr[pos].ref = pos;
}

void heap_update(struct HeapItem* arr, size_t pos, size_t len){

    if (pos > 0 && arr[heap_parent(pos)].expiration_time > arr[pos].expiration_time){
        heap_up(arr, pos);
    }
    else {
        heap_down(arr, pos, len);
    }
}

int heap_init(struct Heap* heap, size_t cap){
    heap->arr = malloc(sizeof(struct HeapItem) * cap);
    if (heap->arr == NULL){
        msg("heap_init (heap.c): malloc");
        return 0;
    }

    heap->cap = cap;
    heap->sz = 0;
    return 1;
}

void heap_free(struct Heap* heap){
    free(heap->arr);
}

static int heap_resize(struct Heap* heap){
    heap->cap *= 2;
    struct HeapItem* new_arr = realloc(heap->arr, sizeof(struct HeapItem) * heap->cap);
    if (new_arr == NULL){
        msg("heap_resize (heap.c): realloc");
        return 0;
    }
    heap->arr = new_arr;
    return 1;
}

int heap_insert(struct Heap* heap, uint64_t expiration_time, size_t* ref){
    if (heap->sz == heap->cap){
        if (heap_resize(heap) == 0){
            return 0;
        }
    }

    heap->arr[heap->sz].expiration_time = expiration_time;
    heap->arr[heap->sz].ref = ref;
    heap->sz++;

    heap_update(heap->arr, heap->sz - 1, heap->sz);
    return 1;
}

void heap_delete(struct Heap* heap, size_t pos){
    heap->arr[pos] = heap->arr[heap->sz - 1];
    heap->sz--;
    if (pos < heap->sz){
        heap_update(heap->arr, pos, heap->sz);
    }
}