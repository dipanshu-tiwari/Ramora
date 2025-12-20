#include "vtr.h"

int resize_vector(struct vector* vtr){
    void** new_data = realloc(vtr->data, vtr->cap * 2 * sizeof(void*));
    if (new_data == NULL){
        msg("resize_vector (vtr.c): realloc");
        return 0;
    }

    vtr->data = new_data;
    vtr->cap *= 2;
    return 1;
}

int init_vector(struct vector* vtr, size_t sz){
    vtr->data = calloc(sz, sizeof(void *));
    if (vtr->data == NULL){
        msg("init_vector (vtr.c): calloc");
        return 0;
    }
    vtr->sz = 0;
    vtr->cap = sz;
    return 1;
}

void free_vector(struct vector* vtr){
    free(vtr->data);
}

int push_back_vector(struct vector* vtr, void* element){
    int rv = 1;
    while (vtr->sz >= vtr->cap){
        rv &= resize_vector(vtr);
        if (rv == 0){
            break;
        }
    }

    vtr->data[vtr->sz] = element;
    vtr->sz += 1;
    return rv;
}