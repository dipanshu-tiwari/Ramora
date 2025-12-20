#include "response.h"

int response_code(struct buf* buffer, enum ResCode code){
    uint8_t res_code = (uint8_t)code;
    return bufappend(buffer, (void*)&res_code, 1);
}

int response_u32(struct buf* buffer, uint32_t value){
    return bufappend(buffer, &value, 4);
}

int response_null(struct buf* buffer){
    uint8_t tag = TAG_NULL;
    return bufappend(buffer, (void*)&tag, 1);
}

int response_int(struct buf* buffer, int value){
    uint8_t tag = TAG_INT;
    int rv = bufappend(buffer, (void*)&tag, 1);
    rv &= bufappend(buffer, (void*)&value, 4);
    return rv;
}

int response_int64(struct buf* buffer, int64_t value){
    uint8_t tag = TAG_INT64;
    int rv = bufappend(buffer, (void*)&tag, 1);
    rv &= bufappend(buffer, (void*)&value, 8);
    return rv;
}

int response_str(struct buf* buffer, char* str){
    // note: str should be null terminated
    uint8_t tag = TAG_STR;
    int rv = bufappend(buffer, (void*)&tag, 1);
    uint32_t len = (uint32_t)strlen(str);
    rv &= response_u32(buffer, len);
    rv &= bufappend(buffer, str, len);
    return rv;
}

int response_str_len(struct buf* buffer, char* str, size_t len){
    // note: str should be null terminated
    uint8_t tag = TAG_STR;
    int rv = bufappend(buffer, (void*)&tag, 1);
    rv &= response_u32(buffer, len);
    rv &= bufappend(buffer, str, len);
    return rv;
}

int response_arr(struct buf* buffer, uint32_t len){
    uint8_t tag = TAG_ARR;
    int rv = bufappend(buffer, (void*)&tag, 1);
    rv &= response_u32(buffer, len);
    return rv;
}