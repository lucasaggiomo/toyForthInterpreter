#include "utils.h"

#include <stdio.h>     // debug, va rimosso
#include <stdlib.h>

/* =============================== ALLOCATION WRAPPERS =============================== */

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("Error in malloc");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *tmp = realloc(ptr, size);
    if (tmp == NULL) {
        perror("Error in realloc");
        exit(1);
    }
    return tmp;
}