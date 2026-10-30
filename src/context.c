#include "context.h"

#include "da.h"
#include "tfstd.h"
#include "utils.h"

#define TF_STD_CALLBACK(name_str, func_ptr) { \
    .name = SV(name_str),                     \
    .kind = TF_EXECUTABLE_CALLBACK,           \
    .as.callback = (func_ptr)                 \
}

// clang-format off
/** Maps the name of the function into the callback */
static TfFunctionEntry TF_STANDARD_LIBRARY[] = {
    TF_STD_CALLBACK("nop",    tfNop),
    TF_STD_CALLBACK("print",  tfPrint),
    TF_STD_CALLBACK("dup",    tfDup),
    TF_STD_CALLBACK("+",      tfAdd),
    TF_STD_CALLBACK("if",     tfIf),
    TF_STD_CALLBACK("ifelse", tfIfelse),
    TF_STD_CALLBACK("call",   tfCall),
    TF_STD_CALLBACK(">",      tfGreater),
    TF_STD_CALLBACK("<",      tfLess),
    TF_STD_CALLBACK(">=",     tfGreaterEq),
    TF_STD_CALLBACK("<=",     tfLessEq),
    TF_STD_CALLBACK("==",     tfEquals),
    // TF_BUILTIN("=",      tfAssign)
};
// clang-format on

#define TF_STANDARD_LIBRARY_COUNT (sizeof(TF_STANDARD_LIBRARY) / sizeof(TF_STANDARD_LIBRARY[0]))

#undef TF_STD_CALLBACK

void initCtx(TfCtx *ctx) {
    ctx->stack = createList();

    // populates the function table with the std library
    ctx->funTable = xmalloc(sizeof(*ctx->funTable));
    da_init(ctx->funTable);

    for (size_t i = 0; i < TF_STANDARD_LIBRARY_COUNT; ++i) {
        da_append(ctx->funTable, &TF_STANDARD_LIBRARY[i]);
    }
}

void deinitCtx(TfCtx *ctx) {
    releaseWord(ctx->stack);
    da_free(ctx->funTable);
    free(ctx->funTable);
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