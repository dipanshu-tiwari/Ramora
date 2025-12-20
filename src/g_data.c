#include "g_data.h"

int init_g_data(struct g_data* gd){
    int rv = init_HMap(&gd->kv_db, INITIAL_HMAP_CAP);
    if (rv == 0){
        die("init_g_data (g_data.c): cannot initialize hmap");
        return 0;
    }

    dlist_init(&gd->idle_list);
    rv &= heap_init(&gd->ttl_heap, INITIAL_HEAP_CAP);
    if (rv == 0){
        free_HMap(&gd->kv_db);
        die("init_g_data (g_data.c): cannot initialize heap");
        return 0;
    }
    
    dlist_init(&gd->conn_pool);
    gd->pooled_conn_count = 0;
    return 1;
}