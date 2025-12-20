#include "cmd_proc.h"

struct CommandSpec command_list[] = {
    {"PING", 1, handle_ping},
    {"GET", 2, handle_get},
    {"SET", 3, handle_set},
    {"DEL", 2, handle_del},
    {"TTL", 3, handle_set_ttl_sec},
    {"TTL", 2, handle_get_ttl_sec},
    {"TTLMS", 3, handle_set_ttl_ms},
    {"TTLMS", 2, handle_get_ttl_ms}
};

size_t command_list_len = 8;

char* get_string(struct buf* buffer, size_t* offset){
    uint32_t len;
    bufcpylenoffset(buffer, *offset, &len, HEADER_SIZE);

    char* str = malloc(sizeof(char) * (len + 1));
    if (str == NULL){
        die("get_string (cmd_proc.c): malloc");
        exit(EXIT_FAILURE);
    }
    *offset += HEADER_SIZE;
    bufcpylenoffset(buffer, *offset, str, len);
    str[len] = '\0';
    *offset += len;

    return str;
}

static int64_t parse_to_int(char* str){
    int is_neg = 0;
    if (*str == '-'){
        is_neg = 1;
        str++;
    }
    int64_t val = 0;
    while (*str){
        if (*str < '0' || *str > '9'){
            return -2; // -2 for incorrect type
        }

        val *= 10;
        val += *str - '0';
        str++;
    }

    if (is_neg == 1){
        val *= -1;
    }

    if (val < 0) return -1;
    else return val;
}

int handle_ping(struct Conn* conn, size_t offset){
    int rv = response_code(conn->wbuf, RES_OK);
    rv &= response_u32(conn->wbuf, 1);
    rv &= response_str(conn->wbuf, "PONG");
    return rv;
}

int handle_get(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);

    struct buf* val = get_Entry(key);
    free(key);
    
    if (val == NULL){
        int rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
        return rv;
    }

    int rv = response_code(conn->wbuf, RES_OK);
    rv &= response_u32(conn->wbuf, 1);
    rv &= response_str_len(conn->wbuf, bufstart(val), bufsize(val));
    return rv;
}

int handle_set(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    char* val = get_string(conn->rbuf, &offset);
    int rv = set_Entry(key, val);
    free(key);
    free(val);

    if (rv == 0) {
        rv = response_code(conn->wbuf, RES_ERR);
    }
    else {
        rv = response_code(conn->wbuf, RES_OK);
    }
    rv &= response_u32(conn->wbuf, 0);
    return rv;
}

int handle_del(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    int rv = del_Entry(key);
    free(key);

    if (rv == 0){
        rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
        return rv;
    }
    rv = response_code(conn->wbuf, RES_OK);
    rv &= response_u32(conn->wbuf, 0);
    return rv;
}

int handle_get_ttl_sec(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    int64_t ttl = get_ttl_Entry_sec(key);
    free(key);

    int rv = 1;
    if (ttl == -2){
        rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
    }
    else {
        rv = response_code(conn->wbuf, RES_OK);
        rv &= response_u32(conn->wbuf, 1);
        rv &= response_int64(conn->wbuf, ttl);
    }
    return rv;
}

int handle_get_ttl_ms(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    int64_t ttl = get_ttl_Entry_ms(key);
    free(key);

    int rv = 1;
    if (ttl == -2){
        rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
    }
    else {
        rv = response_code(conn->wbuf, RES_OK);
        rv &= response_u32(conn->wbuf, 1);
        rv &= response_int64(conn->wbuf, ttl);
    }
    return rv;
}

int handle_set_ttl_sec(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    char* ttl_string = get_string(conn->rbuf, &offset);
    int64_t ttl = parse_to_int(ttl_string);

    if (ttl == -2){
        int rv = 1;
        rv = response_code(conn->wbuf, RES_TY);
        rv &= response_u32(conn->wbuf, 0);
        free(key);
        free(ttl_string);
        return rv;
    }

    int res = set_ttl_Entry_sec(key, ttl);
    free(key);
    free(ttl_string);

    int rv = 1;
    if (res == -1){
        rv = response_code(conn->wbuf, RES_ERR);
        rv &= response_u32(conn->wbuf, 0);
    }
    else if (res == 0){
        rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
    }
    else {
        rv = response_code(conn->wbuf, RES_OK);
        rv &= response_u32(conn->wbuf, 0);
    }
    return rv;
}

int handle_set_ttl_ms(struct Conn* conn, size_t offset){
    char* key = get_string(conn->rbuf, &offset);
    char* ttl_string = get_string(conn->rbuf, &offset);
    int64_t ttl = parse_to_int(ttl_string);

    if (ttl == -2){
        int rv = 1;
        rv = response_code(conn->wbuf, RES_TY);
        rv &= response_code(conn->wbuf, 0);
        free(key);
        free(ttl_string);
        return rv;
    }

    int res = set_ttl_Entry_ms(key, ttl);
    free(key);
    free(ttl_string);

    int rv = 1;
    if (res == 0){
        rv = response_code(conn->wbuf, RES_ERR);
        rv &= response_u32(conn->wbuf, 0);
    }
    else if (res == 0){
        rv = response_code(conn->wbuf, RES_NX);
        rv &= response_u32(conn->wbuf, 0);
    }
    else {
        rv = response_code(conn->wbuf, RES_OK);
        rv &= response_u32(conn->wbuf, 0);
    }
    return rv;
}