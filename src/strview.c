#include "strview.h"
#include <string.h>

#define min(a, b) ((a) <= (b) ? (a) : (b))

int svCompare(const StringView *sv1, const StringView *sv2) {
    if (sv1 == NULL && sv2 == NULL) return 0;
    if (sv1 == NULL) return -1;
    if (sv2 == NULL) return 1;

    size_t prefixLen = min(sv1->len, sv2->len);
    int prefixCmp = memcmp(sv1->data, sv2->data, prefixLen);

    if (prefixCmp == 0) {
        // same prefix, the shorter is the smaller
        if (sv1->len == sv2->len) return 0;
        return sv1->len < sv2->len
            ? -1
            : 1;
    }

    // different prefix, wins the smaller one
    return prefixCmp < 0 ? -1 : 1;
}