#include "tfstd.h"

void tfMonaryFun(TfCtx *ctx, const char *funName, TfMonaryFun fun, TfType typeMaskOp) {
    TF_REQUIRE(ctx->stack->as.list.len >= 1, "Tried %s operation when the stack had less than 1 element", funName);

    TfWord *op = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tfRequireType(funName, op, typeMaskOp, 1);

    fun(ctx, op);

    releaseWord(op);
}

void tfBinaryFun(TfCtx *ctx, const char *funName, TfBinaryFun fun, TfType typeMaskOp1, TfType typeMaskOp2) {
    TF_REQUIRE(ctx->stack->as.list.len >= 2, "Tried %s operation when the stack had less than 2 elements", funName);

    TfWord *op1 = pop(ctx);
    TfWord *op2 = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tfRequireType(funName, op1, typeMaskOp1, 1);
    tfRequireType(funName, op2, typeMaskOp2, 2);

    fun(ctx, op1, op2);

    releaseWord(op1);
    releaseWord(op2);
}

void tfTernaryFun(TfCtx *ctx, const char *funName, TfTernaryFun fun, TfType typeMaskOp1, TfType typeMaskOp2, TfType typeMaskOp3) {
    TF_REQUIRE(ctx->stack->as.list.len >= 3, "Tried %s operation when the stack had less than 3 elements", funName);

    TfWord *op1 = pop(ctx);
    TfWord *op2 = pop(ctx);
    TfWord *op3 = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tfRequireType(funName, op1, typeMaskOp1, 1);
    tfRequireType(funName, op2, typeMaskOp2, 2);
    tfRequireType(funName, op3, typeMaskOp3, 3);

    fun(ctx, op1, op2, op3);

    releaseWord(op1);
    releaseWord(op2);
    releaseWord(op3);
}

void tfNaryFun(TfCtx *ctx, const char *funName, size_t argc, TfNaryFun fun, TfType *typeMaskOp) {
    TF_REQUIRE(argc <= MAX_VARIADIC_ARGS, "Too many arguments for %s: %zu (max %d)", funName, argc, MAX_VARIADIC_ARGS);

    TF_REQUIRE(ctx->stack->as.list.len >= argc, "Tried %s operation when the stack had less than %zu elements", funName, argc);

    TfWord *op[MAX_VARIADIC_ARGS];
    for (size_t i = 0; i < argc; i++) {
        op[i] = pop(ctx);
        tfRequireType(funName, op[i], typeMaskOp[i], i + 1);
    }

    fun(ctx, argc, op);

    for (size_t i = 0; i < argc; i++) {
        releaseWord(op[i]);
    }
}

/* =============================== HELPER FUNCTIONS =============================== */

void printWord(TfWord *word, int indentation) {
    // for (int j = 0; j < indentation; j++) printf("\t");

    // printf("[%s] {refcount = %d} = ", getTypeName(word->type), word->refcount);
    switch (word->type) {
        case TF_NUMBER:
            printf("%d ", word->as.num);
            break;

        case TF_BOOLEAN:
            printf("%s ", word->as.num == 0 ? "false" : "true");
            break;

        case TF_STRING:
            printf("\"" SV_FMT "\" ", SV_ARGS(&word->as.str));
            break;

        case TF_FUNCTION:
            printf("\'" SV_FMT "\' ", SV_ARGS(&word->as.str));
            break;

        case TF_LIST:
            printf("[ ");
            // printf("\n");
            for (size_t i = 0; i < word->as.list.len; i++) {
                printWord(word->as.list.words[i], indentation + 1);     // recursively prints the list
            }

            for (int j = 0; j < indentation; j++)
                printf("\t");
            printf("] ");
            break;

        default:
            fprintf(stderr, "Error in printWord: invalid word type encountered: %d\n", word->type);
            exit(1);
    }
    // printf("\n");
}

void printListFIFO(TfWord *l) {
    for (size_t i = 0; i < l->as.list.len; i++) {
        printWord(l->as.list.words[i], 0);
    }
}

void printStack(TfWord *stack) {
    for (size_t i = 1; i <= stack->as.list.len; i++) {
        printWord(stack->as.list.words[stack->as.list.len - i], 0);
    }
}

void _tfAddFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *sum = createNumber(op1->as.num + op2->as.num);

    push(ctx, sum);
}

void _tfCallFun(TfCtx *ctx, TfWord *program) {
    execute(ctx, program);
}

/** condition [ifTrue] if */
void _tfIfFun(TfCtx *ctx, TfWord *ifTrue, TfWord *expression) {
    // evaluates the expression
    TfWord *cond;
    int toRelease = 0;
    if (expression->type == TF_LIST) {
        // (expression) TF_LIST
        // if the expression is a list, it is executed and the result is popped from the stack
        // the condition is true if the result is positive, false otherwise
        execute(ctx, expression);
        cond = pop(ctx);
        toRelease = 1;
    } else {
        // (literal) TF_BOOLEAN | TF_NUMBER
        cond = expression;
    }

    // execution
    if (cond->as.num) {
        execute(ctx, ifTrue);
    }

    if (toRelease)
        releaseWord(cond);
}

/** condition [ifTrue] [ifFalse] ifelse */
void _tfIfelseFun(TfCtx *ctx, TfWord *ifFalse, TfWord *ifTrue, TfWord *expression) {
    // evaluates the expression
    TfWord *cond;
    int toRelease = 0;
    if (expression->type == TF_LIST) {
        // (expression) TF_LIST
        // if the expression is a list, it is executed and the result is popped from the stack
        // the condition is true if the result is positive, false otherwise
        execute(ctx, expression);
        cond = pop(ctx);
        toRelease = 1;
    } else {
        // (literal) TF_BOOLEAN | TF_NUMBER
        cond = expression;
    }

    // execution
    if (cond->as.num) {
        execute(ctx, ifTrue);
    } else {
        execute(ctx, ifFalse);
    }

    if (toRelease)
        releaseWord(cond);
}

void _tfGreaterFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *result = createBoolean(op1->as.num > op2->as.num);
    push(ctx, result);
}

void _tfLessFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *result = createBoolean(op1->as.num < op2->as.num);
    push(ctx, result);
}

void _tfGreaterEqFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *result = createBoolean(op1->as.num >= op2->as.num);
    push(ctx, result);
}

void _tfLessEqFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *result = createBoolean(op1->as.num <= op2->as.num);
    push(ctx, result);
}

void _tfEqualsFun(TfCtx *ctx, TfWord *op1, TfWord *op2) {
    TfWord *result = createBoolean(op1->as.num == op2->as.num);
    push(ctx, result);
}

/* =============================== TF FUNCTIONS =============================== */
// These are the functions defined in the "standard library" of ToyForth

void tfPrint(TfCtx *ctx) {
    printf("=== STACK === (len = %zu)\n", ctx->stack->as.list.len);
    printStack(ctx->stack);
    printf("\n=============\n");
}

void tfNop(TfCtx *ctx) {
    (void)ctx;
}

void tfDup(TfCtx *ctx) {
    TF_REQUIRE(
        ctx->stack->as.list.len > 0,
        "Tried duplicate operation when the stack was empty"
    );

    TfWord *top = peek(ctx);     // peek does automatic retain -> i need to release the word after usage
    push(ctx, top);              // push does automatic release from me and retain for the stack -> the ownership is transferred to the stack

    /**
     * NOTE: the word 'top' had already been retained by the stack (because it hasn't been removed from the top of it).
     * Therefore, in the end the stack is retaining twice the same word.
     * This is right, because the same reference to the word 'top' is contained two times in the stack:
     * - if the first reference is popped (and then released), the word 'top' won't be freed, because there will still be another
     *   reference available in the stack.
     */
}