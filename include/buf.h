#ifndef BUF_H
#define BUF_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "logging_helper.h"
#include "config.h"

struct buf{
    char* data;
    size_t size;
    size_t cap;
    uint32_t startindex;
};

struct buf* newbuf(size_t cap);
void freebuf(struct buf* buffer);
void clearbuf(struct buf* buffer);
size_t bufsize(struct buf* buffer);
size_t bufavail(struct buf* buffer);
size_t bufcap(struct buf* buffer);

char* bufstart(struct buf* buffer);
char* bufend(struct buf* buffer);

void bufshifttostart(struct buf* buffer);
void bufautoshifttostart(struct buf* buffer);
int bufdoubleonfull(struct buf* buffer);
int bufmakespacelen(struct buf* buffer, size_t len);
int bufmakespace(struct buf* buffer);

void bufconsume(struct buf* buffer, size_t len);
int bufappend(struct buf* buffer, const void* src, size_t len);
void bufcpylen(struct buf* buffer, void* dest, size_t len);
void bufcpylenoffset(struct buf* buffer, size_t offset, void* dest, size_t len);

int bufstrcmp(struct buf* buffer, const char* __str, size_t len);
int bufcmp(struct buf* __buf1, struct buf* __buf2);

#endif