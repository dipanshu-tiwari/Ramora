#include "buf.h"

// 1 -> success
// 0 -> failure

struct buf* newbuf(size_t cap){
    struct buf* buffer = malloc(sizeof(struct buf));
    if (buffer == NULL){
        msg("newbuf (buf.c): malloc");
        return NULL;
    }

    buffer->size = 0;
    buffer->startindex = 0;
    buffer->cap = cap;
    buffer->data = (char*)malloc(sizeof(char) * (cap > 0 ? cap : (size_t)INITIAL_BUFFER_CAP));
    if (buffer->data == NULL){
        free(buffer);
        msg("newbuf (buf.c): malloc");
        return NULL;
    }
    return buffer;
}

void freebuf(struct buf* buffer){
    free(buffer->data);
    free(buffer);
}

void clearbuf(struct buf* buffer){
    buffer->startindex = buffer->size = 0;
}

size_t bufsize(struct buf* buffer){
    return buffer->size;
}

size_t bufavail(struct buf* buffer){
    return buffer->cap - buffer->startindex - buffer->size;
}

size_t bufcap(struct buf* buffer){
    return buffer->cap;
}

char* bufstart(struct buf* buffer){
    return buffer->data + buffer->startindex;
}

char* bufend(struct buf* buffer){
    return buffer->data + buffer->startindex + buffer->size;
}

void bufshifttostart(struct buf* buffer){
    memmove(buffer->data, bufstart(buffer), bufsize(buffer));
    buffer->startindex = 0;
}

void bufautoshifttostart(struct buf* buffer){
    if (buffer->startindex > bufavail(buffer)) bufshifttostart(buffer);
}

int bufrealloc(struct buf* buffer, size_t len){

    char* tmp = (char*)realloc(buffer->data, len);
    if (tmp == NULL){
        msg("bufrealloc (buf.c): realloc");
        return 0;
    }
    buffer->data = tmp;
    buffer->cap = len;

    return 1;
}

int bufdoubleonfull(struct buf* buffer){
    if (bufavail(buffer) == 0){
        size_t newcap = bufcap(buffer) * 2;
        if (newcap > (size_t)MAX_BUFFER_CAP){
            msg("bufdoubleonfull (buf.c): expected length exceeded maximum allowed capacity");
            return 0;
        }
        
        return bufrealloc(buffer, newcap);
    }
    return 1;
}

int bufmakespacelen(struct buf* buffer, size_t len){
    bufautoshifttostart(buffer);
    if (bufavail(buffer) < len){
        size_t newcap = 2 * bufcap(buffer);
        size_t tightcap = bufcap(buffer) + (len - bufavail(buffer));
        while (tightcap > newcap){
            newcap *= 2;
        }

        if (newcap > (size_t)MAX_BUFFER_CAP){
            msg("bufmakespacelen (buf.c): expected length exceeded maximum allowed capacity");
            return 0;
        }

        return bufrealloc(buffer, newcap);
    }
    return 1;
}

int bufmakespace(struct buf* buffer){
    bufautoshifttostart(buffer);
    return bufdoubleonfull(buffer);
}

void bufconsume(struct buf* buffer, size_t len){
    if (bufsize(buffer) < len){
        len = bufsize(buffer);
    }

    buffer->startindex += len;
    buffer->size -= len;
}

int bufappend(struct buf* buffer, const void* src, size_t len){
    int rv = bufmakespacelen(buffer, len);
    if (rv == 0){
        return 0;
    }

    memcpy(bufend(buffer), src, len);
    buffer->size += len;
    return 1;
}

void bufcpylen(struct buf* buffer, void* dest, size_t len){
    if (len == 0 || bufsize(buffer) < len){
        len = bufsize(buffer);
    }

    memcpy(dest, bufstart(buffer), len);
}

void bufcpylenoffset(struct buf* buffer, size_t offset, void* dest, size_t len){
    if (bufsize(buffer) - offset < len){
        len = bufsize(buffer) - offset;
    }

    memcpy(dest, bufstart(buffer) + offset, len);
}

int bufstrcmp(struct buf* buffer, const char* __str, size_t len){
    if (bufsize(buffer) < len) return 1;
    return memcmp(buffer->data, __str, len);
}

int bufcmp(struct buf* __buf1, struct buf* __buf2){
    if (bufsize(__buf1) != bufsize(__buf2)) return 1;
    return memcmp(__buf1->data, __buf2->data, bufsize(__buf1));
}