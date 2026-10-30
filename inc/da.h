#ifndef _DA_H_
#define _DA_H_

/** tsoding-style dynamic array */

#include <stddef.h>

/* =============================== DYNAMIC ARRAY =============================== */
/**
 * Assuming this structure:
 * struct {
 *      size_t capacity;
 *      size_t count;
 *      item_type *items;
 * };
 */

/** Convenient macro to define a new dynamic array struct (with typedef) */
#define DA_DEFINE(Type)  \
    typedef struct {     \
        size_t capacity; \
        size_t count;    \
        Type *items;     \
    }

/** Convenient macro to define a new dynamic array struct (without typedef) */
#define DA_DEFINE_NO_TYPEDEF(name, Type) \
    struct name {                        \
        size_t capacity;                 \
        size_t count;                    \
        Type *items;                     \
    }

#ifndef DA_REALLOCARRAY
#include <stdlib.h>
#define DA_REALLOCARRAY reallocarray
#endif

#ifndef DA_FREE
#include <stdlib.h>
#define DA_FREE free
#endif

#define DA_INITIAL_CAPACITY 2

#define da_reserve(da, cap)                                                               \
    do {                                                                                  \
        if (cap <= (da)->capacity)                                                        \
            break;                                                                        \
        if ((da)->capacity == 0)                                                          \
            (da)->capacity = DA_INITIAL_CAPACITY;                                         \
        while (cap > (da)->capacity)                                                      \
            (da)->capacity *= 2;                                                          \
        (da)->items = DA_REALLOCARRAY((da)->items, (da)->capacity, sizeof(*(da)->items)); \
    } while (0)

#define da_append(da, item)                  \
    do {                                     \
        da_reserve(da, (da)->count + 1);     \
        (da)->count += 1;                    \
        (da)->items[(da)->count - 1] = item; \
    } while (0)

#define da_init(da)         \
    do {                    \
        (da)->capacity = 0; \
        (da)->count = 0;    \
        (da)->items = 0;    \
    } while (0)

#define da_free(da)                  \
    do {                             \
        if (!(da) || !((da)->items)) \
            break;                   \
        DA_FREE((da)->items);        \
        (da)->items = NULL;          \
        (da)->capacity = 0;          \
        (da)->count = 0;             \
    } while (0)

// probably could do it with typeof but this is more readable when used
/** 'it' is a pointer to the current item */
#define da_foreach(Type, it, da) \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

/** Applies fun(item) to each item in da */
#define da_apply(da, fun)                          \
    do {                                           \
        for (size_t i = 0; i < (da)->count; i++) { \
            fun((da)->items[i]);                   \
        }                                          \
    } while (0)

/** Applies fun(item, ctx) to each item in da */
#define da_apply_ctx(da, fun, ctx)                 \
    do {                                           \
        for (size_t i = 0; i < (da)->count; i++) { \
            fun((da)->items[i], ctx);              \
        }                                          \
    } while (0)

#define da_empty(da) ((da)->count == 0)

/** Doesn't check index bounds, it's just a convenient macro to improve readability. */
#define da_get(da, idx) ((da)->items[(idx)])

/** If index out of bounds, returns default value. Assumes idx is always positive (otherwise if its type is size_t there will be annoying -Wtype-limits warnings) */
#define da_get_or_default(da, idx, def) ((((size_t)idx) >= (da)->count) ? def : (da)->items[idx])

#endif     // _DA_H_