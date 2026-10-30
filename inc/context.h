#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "word.h"

typedef struct TfCtx {
    TfWord *stack;     // a stack is a pointer to a TfWord of type TF_LIST
} TfCtx;

void initCtx(TfCtx *ctx);

void deinitCtx(TfCtx *ctx);

/**
 * Pushes the 'word' on the stack.
 * See listPush for more details.
 */
void push(TfCtx *ctx, TfWord *word);

/**
 * Pops the word on the top of the stack.
 * See listPop for more details.
 */
TfWord *pop(TfCtx *ctx);

/**
 * Peeks the word on the top of the stack (like pop but it is not removed from the stack).
 * See listPeek for more details.
 */
TfWord *peek(TfCtx *ctx);

#endif		// _CONTEXT_H_