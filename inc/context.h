#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "word.h"

struct tf_ctx {
    tf_word *stack;     // a stack is a pointer to a tf_word of type TF_LIST
};
typedef struct tf_ctx tf_ctx;

void initCtx(tf_ctx *ctx);

void deinitCtx(tf_ctx *ctx);

/**
 * Pushes the 'word' on the stack.
 * See listPush for more details.
 */
void push(tf_ctx *ctx, tf_word *word);

/**
 * Pops the word on the top of the stack.
 * See listPop for more details.
 */
tf_word *pop(tf_ctx *ctx);

/**
 * Peeks the word on the top of the stack (like pop but it is not removed from the stack).
 * See listPeek for more details.
 */
tf_word *peek(tf_ctx *ctx);

#endif		// _CONTEXT_H_