#ifndef REF_H
#define REF_H

#include "stdlib.h"
#include "stddef.h"

/* From https://nullprogram.com/blog/2015/02/17/ */

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct refs {
    void (*free)(const struct refs *);
    int count;
};

static inline void ref_inc(const struct refs *r) {
    ((struct refs *)r)->count++;
}

static inline void ref_dec(const struct refs *r) {
    if (--((struct refs *)r)->count == 0) {
        r->free(r);
    }
}

#endif

