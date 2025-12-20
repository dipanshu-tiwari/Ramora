#ifndef OPER_H
#define OPER_H

#include <stddef.h>

#include "response.h"
#include "g_data.h"
#include "timer.h"

extern struct g_data gd;

struct buf* get_Entry(char* key);
int set_Entry(char* key, char* val);
int del_Entry(char* key);
int set_ttl_Entry_ms(char* key, int64_t ttl);
int set_ttl_Entry_sec(char* key, int64_t ttl);
int64_t get_ttl_Entry_ms(char* key);
int64_t get_ttl_Entry_sec(char* key);

#endif