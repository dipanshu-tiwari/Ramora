#ifndef VTR_H
#define VTR_H

#include <stdlib.h>
#include <stdio.h>

#include "logging_helper.h"

struct vector {
    void** data;
    size_t sz;
    size_t cap;
};

int init_vector(struct vector* vtr, size_t sz);
void free_vector(struct vector* vtr);
int resize_vector(struct vector* vtr);
int push_back_vector(struct vector* vtr, void* element);

#endif