#ifndef _STRING_VIEW_H_
#define _STRING_VIEW_H_

#include <stddef.h>
#include <strbuf.h>
#include <string.h>

/**
 * Points to a string without owning it.
 * The string doesn't need to be NULL terminated.
 */
typedef struct StringView {
    size_t len;
    const char *data;
} StringView;

/**
 * Creates a new StringView from a Literal string `lit`.
 * Uses 'contatenation trick' `""(lit)` to assert at compile time that `lit` is a literal (string concatenation works only with literals)
 */
#define SV(lit)                    \
    ((StringView) {                \
        .data = "" lit,            \
        .len  = sizeof("" lit) - 1 \
    })

/** Creates a new StringView from a cstr (NULL terminated string), using strlen. */
static inline StringView svFromCstr(const char *s) {
    return (StringView) {
        .len = strlen(s),
        .data = s
    };
}

/** Creates a new StringView from a StringBuf. */
static inline StringView svFromSb(StringBuf *sb) {
    return (StringView) {
        .len = sb->len,
        .data = sb->data
    };
}

#define SV_FMT "%.*s"
#define SV_ARGS(sv) (int)((sv)->len), (sv)->data

/**
 * Compares two StringViews, returning:
 * -  0: if the strings are equal
 * - -1: if sv1 < sv2
 * -  1: if sv1 > sv2
 *
 * A NULL StringView is considered smaller than everything, so:
 * - svCompare(NULL, NULL)     returns  0
 * - svCompare(NULL, anything) returns -1
 * - svCompare(anything, NULL) returns  1
 *
 * The comparison is made with memcmp and refers to the pointed string StringView::data.
 */
int svCompare(const StringView *sv1, const StringView *sv2);

#endif /* _STRING_VIEW_H_ */