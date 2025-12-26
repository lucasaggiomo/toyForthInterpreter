#include "execution.h"

#include "context.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =============================== UTILS =============================== */

// aborts the execution due to a semantic error of the ToyForth program
#define ABORT_EXECUTION(fmt, ...)               \
    do {                                        \
        fprintf(stderr, "EXECUTION FAILURE: "); \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        fprintf(stderr, "\n");                  \
        exit(EXIT_FAILURE);                     \
    } while (0)

// an assert-like instruction that is convenient to reduce boilerplate code in functions
#define TF_REQUIRE(expr, fmt, ...)                        \
    do {                                                  \
        if (!(expr)) ABORT_EXECUTION(fmt, ##__VA_ARGS__); \
    } while (0)

static inline void tf_require_type(
    const char *funName,
    tf_word *op,
    tf_type mask,
    size_t num) {
    if (mask != TF_UNKNOWN) {
        TF_REQUIRE(
            TF_VALID_TYPE(op, mask),
            "Tried %s operation where operand %zu was not of the expected type "
            "(expected: %s, actual: %s)",
            funName,
            num,
            getTypeName(mask),
            getTypeName(op->type));
    }
}

typedef void (*tf_monary_fun)(tf_ctx *, tf_word *);
typedef void (*tf_binary_fun)(tf_ctx *, tf_word *, tf_word *);
typedef void (*tf_ternary_fun)(tf_ctx *, tf_word *, tf_word *, tf_word *);
typedef void (*tf_nary_fun)(tf_ctx *, size_t, tf_word **);

void tfmonaryfun(tf_ctx *ctx, const char *funName, tf_type typeMaskOp, tf_monary_fun fun) {
    TF_REQUIRE(ctx->stack->list.len >= 1,
               "Tried %s operation when the stack had less than 1 element", funName);

    tf_word *op = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tf_require_type(funName, op, typeMaskOp, 1);

    fun(ctx, op);

    releaseWord(op);
}

void tfbinaryfun(tf_ctx *ctx, const char *funName, tf_type typeMaskOp1, tf_type typeMaskOp2, tf_binary_fun fun) {
    TF_REQUIRE(ctx->stack->list.len >= 2,
               "Tried %s operation when the stack had less than 2 elements", funName);

    tf_word *op1 = pop(ctx);
    tf_word *op2 = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tf_require_type(funName, op1, typeMaskOp1, 1);
    tf_require_type(funName, op2, typeMaskOp2, 2);

    fun(ctx, op1, op2);

    releaseWord(op1);
    releaseWord(op2);
}

void tfternaryfun(tf_ctx *ctx, const char *funName, tf_type typeMaskOp1, tf_type typeMaskOp2, tf_type typeMaskOp3, tf_ternary_fun fun) {
    TF_REQUIRE(ctx->stack->list.len >= 3,
               "Tried %s operation when the stack had less than 3 elements", funName);

    tf_word *op1 = pop(ctx);
    tf_word *op2 = pop(ctx);
    tf_word *op3 = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tf_require_type(funName, op1, typeMaskOp1, 1);
    tf_require_type(funName, op2, typeMaskOp2, 2);
    tf_require_type(funName, op3, typeMaskOp3, 3);

    fun(ctx, op1, op2, op3);

    releaseWord(op1);
    releaseWord(op2);
    releaseWord(op3);
}

#define MAX_VARIADIC_ARGS 16

void tfnaryfun(tf_ctx *ctx, const char *funName, size_t argc, tf_type *typeMaskOp, tf_nary_fun fun) {
    TF_REQUIRE(argc <= MAX_VARIADIC_ARGS,
               "Too many arguments for %s: %zu (max %d)",
               funName, argc, MAX_VARIADIC_ARGS);

    TF_REQUIRE(ctx->stack->list.len >= argc,
               "Tried %s operation when the stack had less than %zu elements", funName, argc);

    tf_word *op[MAX_VARIADIC_ARGS];
    for (size_t i = 0; i < argc; i++) {
        op[i] = pop(ctx);
        tf_require_type(funName, op[i], typeMaskOp[i], i + 1);
    }

    fun(ctx, argc, op);

    for (size_t i = 0; i < argc; i++) {
        releaseWord(op[i]);
    }
}

// convenient macros to define new functions

#define TF_DEF_MONARY(name, identifier, typeMaskOp)                \
    static inline void tf##name(tf_ctx *ctx) {                     \
        tfmonaryfun(ctx, #identifier, typeMaskOp, _tf##name##fun); \
    }

#define TF_DEF_BINARY(name, identifier, typeMaskOp1, typeMaskOp2)                \
    static inline void tf##name(tf_ctx *ctx) {                                   \
        tfbinaryfun(ctx, #identifier, typeMaskOp1, typeMaskOp2, _tf##name##fun); \
    }

#define TF_DEF_TERNARY(name, identifier, typeMaskOp1, typeMaskOp2, typeMaskOp3)                \
    static inline void tf##name(tf_ctx *ctx) {                                                 \
        tfternaryfun(ctx, #identifier, typeMaskOp1, typeMaskOp2, typeMaskOp3, _tf##name##fun); \
    }

/* =============================== HELPER FUNCTIONS =============================== */

void printWord(tf_word *word, int indentation) {
    // for (int j = 0; j < indentation; j++) printf("\t");

    // printf("[%s] {refcount = %d} = ", getTypeName(word->type), word->refcount);
    switch (word->type) {
        case TF_NUMBER:
            printf("%d ", word->num);
            break;
        case TF_BOOLEAN:
            printf("%s ", word->num == 0 ? "false" : "true");
            break;
        case TF_STRING:
            printf("\"%s\" ", word->str.ptr);
            break;
        case TF_FUNCTION:
            printf("\'%s\' ", word->str.ptr);
            break;
        case TF_LIST:
            printf("[ ");
            // printf("\n");
            for (size_t i = 0; i < word->list.len; i++) {
                printWord(word->list.words[i], indentation + 1);     // recursively prints the list
            }
            for (int j = 0; j < indentation; j++) printf("\t");
            printf("] ");
            break;
        default:
            fprintf(stderr, "Error in printWord: invalid word type encountered: %d\n", word->type);
            exit(1);
    }
    // printf("\n");
}

void printListFIFO(tf_word *l) {
    for (size_t i = 0; i < l->list.len; i++) {
        printWord(l->list.words[i], 0);
    }
}

void printStack(tf_word *stack) {
    for (size_t i = 1; i <= stack->list.len; i++) {
        printWord(stack->list.words[stack->list.len - i], 0);
    }
}

void _tfaddfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *sum = createNumber(op1->num + op2->num);

    push(ctx, sum);
}

void _tfcallfun(tf_ctx *ctx, tf_word *program) {
    execute(ctx, program);
}

// static tf_monary_fun _tfcallfun = execute;

/** condition [ifTrue] if */
void _tfiffun(tf_ctx *ctx, tf_word *ifTrue, tf_word *expression) {
    // evaluates the expression
    tf_word *cond;
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
    if (cond->num) {
        execute(ctx, ifTrue);
    }

    if (toRelease) releaseWord(cond);
}

/** condition [ifTrue] [ifFalse] ifelse */
void _tfifelsefun(tf_ctx *ctx, tf_word *ifFalse, tf_word *ifTrue, tf_word *expression) {
    // evaluates the expression
    tf_word *cond;
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
    if (cond->num) {
        execute(ctx, ifTrue);
    } else {
        execute(ctx, ifFalse);
    }

    if (toRelease) releaseWord(cond);
}

void _tfgreaterfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *result = createBoolean(op1->num > op2->num);

    push(ctx, result);
}

void _tflessfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *result = createBoolean(op1->num < op2->num);

    push(ctx, result);
}

void _tfgreaterEqfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *result = createBoolean(op1->num >= op2->num);

    push(ctx, result);
}

void _tflessEqfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *result = createBoolean(op1->num <= op2->num);

    push(ctx, result);
}

void _tfequalsfun(tf_ctx *ctx, tf_word *op1, tf_word *op2) {
    tf_word *result = createBoolean(op1->num == op2->num);

    push(ctx, result);
}

/* =============================== TF FUNCTIONS =============================== */
// These are the functions define in the "standard library" of ToyForth

/** Prints the stack on the screen. */
void tfprint(tf_ctx *ctx) {
    printf("=== STACK === (len = %zu)\n", ctx->stack->list.len);
    printStack(ctx->stack);
    printf("\n=============\n");
}

/** Does nothing. */
void tfnop(tf_ctx *ctx) {
    (void)ctx;
}

/**
 * Duplicates the word at the top of the stack.
 */
void tfdup(tf_ctx *ctx) {
    TF_REQUIRE(ctx->stack->list.len > 0,
               "Tried duplicate operation when the stack was empty");

    tf_word *top = peek(ctx);     // peek does automatic retain -> i need to release the word after usage
    push(ctx, top);               // push does automatic release from me and retain for the stack -> the ownership is transferred to the stack

    /**
     * NOTE: the word 'top' had already been retained by the stack (because it hasn't been removed from the top of it).
     * Therefore, in the end the stack is retaining twice the same word.
     * This is right, because the same reference to the word 'top' is contained two times in the stack:
     * - if the first reference is popped (and then released), the word 'top' won't be freed, because there will still be another
     *   reference available in the stack.
     */
}

TF_DEF_MONARY(call, call, TF_LIST);

/**
 * Pops two words from the stack and pushes their sum (interpreting them as a TF_NUMBER).
 */
TF_DEF_BINARY(add, +, TF_NUMBER, TF_NUMBER)

// control flow
TF_DEF_BINARY(if, if, TF_LIST, TF_LIST | TF_BOOLEAN | TF_NUMBER)
TF_DEF_TERNARY(ifelse, ifelse, TF_LIST, TF_LIST, TF_LIST | TF_BOOLEAN | TF_NUMBER)

// logical operators
TF_DEF_BINARY(greater, >, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(less, <, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(greaterEq, >=, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(lessEq, <=, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(equals, ==, TF_NUMBER, TF_NUMBER)

/* =============================== EXECUTION LOGIC =============================== */

/** Maps the name of the function into the callback */
static tf_function FUNCTION_TABLE[]
    = {
          { "print", tfprint },
          { "nop", tfnop },
          { "dup", tfdup },
          { "+", tfadd },
          { "if", tfif },
          { "ifelse", tfifelse },
          { "call", tfcall },
          { ">", tfgreater },
          { "<", tfless },
          { ">=", tfgreaterEq },
          { "<=", tflessEq },
          { "==", tfequals },
          //   { "=", tfassign }  // reserved for the future (support variables)
      };

#define FUNCTIONS_COUNT (sizeof(FUNCTION_TABLE) / sizeof(tf_function))

tf_callback getCallback(const char *name) {
    // trivial implementation:
    // - a table contains all the functions, with an asosciated name (string)
    // - getCallback finds the function to execute in the table in linear time (O(n))

    tf_callback callback = NULL;
    int found = 0;
    for (size_t i = 0; i < FUNCTIONS_COUNT; i++) {
        if (strcmp(name, FUNCTION_TABLE[i].name) == 0) {
            callback = FUNCTION_TABLE[i].callback;
            found = 1;
            break;
        }
    }
    if (found == 0) {
        fprintf(stderr, "Error in getCallback: invalid symbol encountered: %s\n", name);
        exit(1);
    }

    return callback;
}

void executeFunction(tf_ctx *ctx, tf_word *fun) {
    assert(fun->type == TF_FUNCTION);

    tf_callback callback = getCallback(fun->str.ptr);
    callback(ctx);
}

void execute(tf_ctx *ctx, tf_word *program) {
    tf_word *curr = NULL;
    for (size_t i = 0; i < program->list.len; i++) {
        curr = program->list.words[i];
        switch (curr->type) {
            case TF_NUMBER:
            case TF_BOOLEAN:
            case TF_STRING:
            case TF_LIST:
                // pushes the word on the stack
                push(ctx, curr);
                retainWord(curr);     // needs to retain the word again to track that 'program' still has got reference to curr
                break;
            case TF_FUNCTION:
                // executes the function with the associated name
                executeFunction(ctx, curr);
                break;
            default:
                fprintf(stderr, "Error in execute: invalid word type encountered: %d\n", curr->type);
                exit(1);
        }
    }
}