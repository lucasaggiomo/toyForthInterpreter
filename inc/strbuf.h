#ifndef _STRING_BUF_H_
#define _STRING_BUF_H_

#include <stddef.h>

#ifndef SB_MALLOC
#include <stdlib.h>
#define SB_MALLOC malloc
#endif

#ifndef SB_FREE
#include <stdlib.h>
#define SB_FREE free
#endif

/**
 * Wraps an owned string pointer, adding length.
 * The string doesn't need to be NULL terminated.
 */
typedef struct StringBuf {
    size_t len;
    char *data;
} StringBuf;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * Creates a new StringBuf from a Literal string `lit`.
 * Uses 'contatenation trick' `""(lit)` to assert at compile time that `lit` is a literal (string concatenation works only with literals)
 */
#define SB(lit)          \
    ((StringBuf) {       \
        .data = ""(lit), \
        .len = (ARRAY_SIZE(lit) - 1) })

/**
 * Adopts `s` (must be malloc'd). StringBuf now owns it.
 * The NULL terminator is not copied.
 */
StringBuf sbAdoptCstr(char *s);

/**
 * Creates a new StringBuf, copying data from `s` in a new malloc`d container StringBuf::data.
 */
StringBuf sbCreate(const char *s, size_t len);

/** Frees the adopted string StringBuf::data and resets `sb`. */
void sbFree(StringBuf *sb);

#define SB_FMT "%.*s"
#define SB_ARGS(sb) (int)((sb)->len), (sb)->data

/**
 * Compares two StringBufs, returning:
 * -  0: if the strings are equal
 * - -1: if sb1 < sb2
 * -  1: if sb1 > sb2
 *
 * A NULL StringBuf is considered smaller than everything, so:
 * - sbCompare(NULL, NULL)     returns  0
 * - sbCompare(NULL, anything) returns -1
 * - sbCompare(anything, NULL) returns  1
 *
 * The comparison is made with memcmp and refers to the pointed string StringBuf::data.
 */
int sbCompare(const StringBuf *sb1, const StringBuf *sb2);

#endif /* _STRING_BUF_H_ */