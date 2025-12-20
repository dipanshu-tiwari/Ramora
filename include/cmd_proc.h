#ifndef CMD_PROC_H
#define CMD_PROC_H

#include "conn.h"
#include "oper.h"

struct CommandSpec {
    const char* cmd;
    uint32_t nstr;
    int (*handler)(struct Conn*, size_t);
};

extern struct CommandSpec command_list[];
extern size_t command_list_len;

char* get_string(struct buf* buffer, size_t* offset);

int handle_ping(struct Conn* conn, size_t offset);
int handle_get(struct Conn* conn, size_t offset);
int handle_set(struct Conn* conn, size_t offset);
int handle_del(struct Conn* conn, size_t offset);
int handle_get_ttl_sec(struct Conn* conn, size_t offset);
int handle_get_ttl_ms(struct Conn* conn, size_t offset);
int handle_set_ttl_sec(struct Conn* conn, size_t offset);
int handle_set_ttl_ms(struct Conn* conn, size_t offset);

#endif