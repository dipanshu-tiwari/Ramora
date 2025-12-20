#ifndef RESPONSE_H
#define RESPONSE_H

#include "buf.h"

enum ResCode {
    RES_OK = 0, // command executed successfully
    RES_ERR = 1, // error occured
    RES_NX = 2, // no entry found
    RES_TY = 3 // type error
};

enum Tag {
    TAG_NULL = 0,
    TAG_INT = 1,
    TAG_STR = 2,
    TAG_ARR = 3,
    TAG_INT64 = 4
};

int response_code(struct buf* buffer, enum ResCode code);
int response_u32(struct buf* buffer, uint32_t value);
int response_null(struct buf* buffer);
int response_int(struct buf* buffer, int value);
int response_int64(struct buf* buffer, int64_t value);
int response_str(struct buf* buffer, char* str);
int response_str_len(struct buf* buffer, char* str, size_t len);
int response_arr(struct buf* buffer, uint32_t len);

#endif