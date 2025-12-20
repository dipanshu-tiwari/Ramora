#ifndef CONN_H
#define CONN_H

#include "timer.h"
#include "g_data.h"

#define HEADER_SIZE 4

extern struct g_data gd;

struct Conn {
    int fd;
    uint8_t want_close;
    struct buf* rbuf;
    struct buf* wbuf;

    uint64_t last_used_time;
    struct DList idle_node;
};

static inline void conn_update_timer(struct Conn* conn){
    dlist_delete(&conn->idle_node);
    dlist_insert_before(&gd.idle_list, &conn->idle_node);
    conn->last_used_time = get_monotonic_msec();
}

struct Conn* create_conn(int fd);
void free_conn(struct Conn* conn);
void hard_free_conn(struct Conn* conn);
int smallest_remaining_time();

#endif