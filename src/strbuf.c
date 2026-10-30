#include <stdlib.h>
#include <string.h>
#include <utils.h>

#define SB_MALLOC xmalloc
#define SB_FREE free

#include "strbuf.h"

#define min(a, b) ((a) <= (b) ? (a) : (b))

StringBuf sbAdoptCstr(char *s) {
    return (StringBuf) {
        .len = s ? strlen(s) : 0,
        .data = s
    };
}

StringBuf sbCreate(const char *s, size_t len) {
    char *data = SB_MALLOC(len);
    memcpy(data, s, len);
    return (StringBuf) {
        .len = len,
        .data = data
    };
}

void sbFree(StringBuf *sb) {
    SB_FREE(sb->data);
    sb->data = NULL;
    sb->len = 0;
}

int sbCompare(const StringBuf *sb1, const StringBuf *sb2) {
    if (sb1 == NULL && sb2 == NULL) return 0;
    if (sb1 == NULL) return -1;
    if (sb2 == NULL) return 1;

    size_t prefixLen = min(sb1->len, sb2->len);
    int prefixCmp = memcmp(sb1->data, sb2->data, prefixLen);

    if (prefixCmp == 0) {
        // same prefix, the shorter is the smaller
        if (sb1->len == sb2->len) return 0;
        return sb1->len < sb2->len
            ? -1
            : 1;
    }

    // different prefix, wins the smaller one
    return prefixCmp < 0 ? -1 : 1;
}