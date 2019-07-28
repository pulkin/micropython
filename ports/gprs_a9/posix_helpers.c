#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include "py/mphal.h"
#include "py/gc.h"
#include <sdk_init.h>

void *malloc(size_t size) {
    void *p = gc_alloc(size, false);
    if (p == NULL) {
        // POSIX requires ENOMEM to be set in case of error
        // errno = ENOMEM;
    }
    return p;
}
void free(void *ptr) {
    gc_free(ptr);
}
void *calloc(size_t nmemb, size_t size) {
    return malloc(nmemb * size);
}
void *realloc(void *ptr, size_t size) {
    void *p = gc_realloc(ptr, size, true);
    if (p == NULL) {
        // POSIX requires ENOMEM to be set in case of error
        // errno = ENOMEM;
    }
    return p;
}

#undef htonl
#undef ntohl
uint32_t ntohl(uint32_t netlong) {
    return MP_BE32TOH(netlong);
}
uint32_t htonl(uint32_t netlong) {
    return MP_HTOBE32(netlong);
}

/* #undef gettimeofday
int gettimeofday(struct timeval *tv, struct timezone *tz) {
    return CSDK_FUNC(gettimeofday)(tv, tz);
} */
