#ifndef G_DATA_H
#define G_DATA_H

#include "hmap.h"
#include "dlist.h"
#include "heap.h"

struct g_data {
    struct HMap kv_db;
    struct DList idle_list;
    struct Heap ttl_heap;
    struct DList conn_pool;
    int pooled_conn_count;
};

int init_g_data(struct g_data* gd);

#endif