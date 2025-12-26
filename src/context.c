#include "context.h"

void initCtx(tf_ctx *ctx) {
    ctx->stack = createList();
}

void deinitCtx(tf_ctx *ctx) {
    releaseWord(ctx->stack);
}

void push(tf_ctx *ctx, tf_word *word) {
    listPush(ctx->stack, word);
}

tf_word *pop(tf_ctx *ctx) {
    return listPop(ctx->stack);
}

tf_word *peek(tf_ctx *ctx) {
    return listPeek(ctx->stack);
}