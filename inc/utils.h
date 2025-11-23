#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>

/* =============================== ALLOCATION WRAPPERS =============================== */

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

#endif     // _UTILS_H_