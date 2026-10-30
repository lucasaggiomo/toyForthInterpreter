#include "context.h"

void initCtx(TfCtx *ctx) {
    ctx->stack = createList();
}

void deinitCtx(TfCtx *ctx) {
    releaseWord(ctx->stack);
}

void push(TfCtx *ctx, TfWord *word) {
    listPush(ctx->stack, word);
}

TfWord *pop(TfCtx *ctx) {
    return listPop(ctx->stack);
}

TfWord *peek(TfCtx *ctx) {
    return listPeek(ctx->stack);
}