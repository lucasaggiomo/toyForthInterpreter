#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "word.h"

struct tf_ctx {
    tf_word *stack;     // a stack is a pointer to a tf_word of type TF_LIST
};
typedef struct tf_ctx tf_ctx;

void initCtx(tf_ctx *ctx);

void deinitCtx(tf_ctx *ctx);

/** Pushes the *tf_word 'word' on the stack.  */
void push(tf_ctx *ctx, tf_word *word);

/** Pops the tf_word on the top on the stack. */
tf_word *pop(tf_ctx *ctx);

#endif		// _CONTEXT_H_