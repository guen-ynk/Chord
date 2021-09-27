#ifndef BLOCK3_HASH_H
#define BLOCK3_HASH_H
#include "uthash.h"
#include "sockUtils.h"

typedef struct _hash_struct {
    buffer* key;
    buffer* value;
    UT_hash_handle hh; /* makes this structure hashable */
} hash_struct;

hash_struct *create(buffer* key, buffer* value);
int set(buffer* key, buffer* value);
hash_struct *get(buffer* key);
int delete(buffer* key);

#endif //BLOCK3_HASH_H
