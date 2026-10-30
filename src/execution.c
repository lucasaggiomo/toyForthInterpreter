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

static inline void tfRequireType(
    const char *funName,
    TfWord *op,
    TfType mask,
    size_t num
) {
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

typedef void (*TfMonaryFun)(TfCtx *, TfWord *);
typedef void (*TfBinaryFun)(TfCtx *, TfWord *, TfWord *);
typedef void (*TfTernaryFun)(TfCtx *, TfWord *, TfWord *, TfWord *);
typedef void (*TfNaryFun)(TfCtx *, size_t, TfWord **);

void tfMonaryFun(TfCtx *ctx, const char *funName, TfType typeMaskOp, TfMonaryFun fun) {
    TF_REQUIRE(ctx->stack->as.list.len >= 1,
               "Tried %s operation when the stack had less than 1 element", funName);

    TfWord *op = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tfRequireType(funName, op, typeMaskOp, 1);

    fun(ctx, op);

    releaseWord(op);
}

void tfBinaryFun(TfCtx *ctx, const char *funName, TfType typeMaskOp1, TfType typeMaskOp2, TfBinaryFun fun) {
    TF_REQUIRE(ctx->stack->as.list.len >= 2,
               "Tried %s operation when the stack had less than 2 elements", funName);

    TfWord *op1 = pop(ctx);
    TfWord *op2 = pop(ctx);

    // checks the required types (only if a specific type is required -> TF_UNKNOWN means no checking)
    tfRequireType(funName, op1, typeMaskOp1, 1);
    tfRequireType(funName, op2, typeMaskOp2, 2);

    fun(ctx, op1, op2);

    releaseWord(op1);
    releaseWord(op2);
}

void tfTernaryFun(TfCtx *ctx, const char *funName, TfType typeMaskOp1, TfType typeMaskOp2, TfType typeMaskOp3, TfTernaryFun fun) {
    TF_REQUIRE(ctx->stack->as.list.len >= 3,
               "Tried %s operation when the stack had less than 3 elements", funName);

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

#define MAX_VARIADIC_ARGS 16

void tfNaryFun(TfCtx *ctx, const char *funName, size_t argc, TfType *typeMaskOp, TfNaryFun fun) {
    TF_REQUIRE(argc <= MAX_VARIADIC_ARGS,
               "Too many arguments for %s: %zu (max %d)",
               funName, argc, MAX_VARIADIC_ARGS);

    TF_REQUIRE(ctx->stack->as.list.len >= argc,
               "Tried %s operation when the stack had less than %zu elements", funName, argc);

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

// convenient macros to define new functions

#define TF_DEF_MONARY(name, identifier, typeMaskOp)                \
    static inline void tf##name(TfCtx *ctx) {                      \
        tfMonaryFun(ctx, #identifier, typeMaskOp, _tf##name##Fun); \
    }

#define TF_DEF_BINARY(name, identifier, typeMaskOp1, typeMaskOp2)                \
    static inline void tf##name(TfCtx *ctx) {                                    \
        tfBinaryFun(ctx, #identifier, typeMaskOp1, typeMaskOp2, _tf##name##Fun); \
    }

#define TF_DEF_TERNARY(name, identifier, typeMaskOp1, typeMaskOp2, typeMaskOp3)                \
    static inline void tf##name(TfCtx *ctx) {                                                  \
        tfTernaryFun(ctx, #identifier, typeMaskOp1, typeMaskOp2, typeMaskOp3, _tf##name##Fun); \
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
            printf("\"%s\" ", word->as.str.ptr);
            break;

        case TF_FUNCTION:
            printf("\'%s\' ", word->as.str.ptr);
            break;

        case TF_LIST:
            printf("[ ");
            // printf("\n");
            for (size_t i = 0; i < word->as.list.len; i++) {
                printWord(word->as.list.words[i], indentation + 1);     // recursively prints the list
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

// static TfMonaryFun _tfCallFun = execute;

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

    if (toRelease) releaseWord(cond);
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

    if (toRelease) releaseWord(cond);
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
// These are the functions define in the "standard library" of ToyForth

/** Prints the stack on the screen. */
void tfPrint(TfCtx *ctx) {
    printf("=== STACK === (len = %zu)\n", ctx->stack->as.list.len);
    printStack(ctx->stack);
    printf("\n=============\n");
}

/** Does nothing. */
void tfNop(TfCtx *ctx) {
    (void)ctx;
}

/**
 * Duplicates the word at the top of the stack.
 */
void tfDup(TfCtx *ctx) {
    TF_REQUIRE(ctx->stack->as.list.len > 0,
               "Tried duplicate operation when the stack was empty");

    TfWord *top = peek(ctx);     // peek does automatic retain -> i need to release the word after usage
    push(ctx, top);               // push does automatic release from me and retain for the stack -> the ownership is transferred to the stack

    /**
     * NOTE: the word 'top' had already been retained by the stack (because it hasn't been removed from the top of it).
     * Therefore, in the end the stack is retaining twice the same word.
     * This is right, because the same reference to the word 'top' is contained two times in the stack:
     * - if the first reference is popped (and then released), the word 'top' won't be freed, because there will still be another
     *   reference available in the stack.
     */
}

TF_DEF_MONARY(Call, call, TF_LIST)

/**
 * Pops two words from the stack and pushes their sum (interpreting them as a TF_NUMBER).
 */
TF_DEF_BINARY(Add, +, TF_NUMBER, TF_NUMBER)

// control flow
TF_DEF_BINARY(If, if, TF_LIST, TF_LIST | TF_BOOLEAN | TF_NUMBER)
TF_DEF_TERNARY(Ifelse, ifelse, TF_LIST, TF_LIST, TF_LIST | TF_BOOLEAN | TF_NUMBER)

// logical operators
TF_DEF_BINARY(Greater, >, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(Less, <, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(GreaterEq, >=, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(LessEq, <=, TF_NUMBER, TF_NUMBER)
TF_DEF_BINARY(Equals, ==, TF_NUMBER, TF_NUMBER)

/* =============================== EXECUTION LOGIC =============================== */

/** Maps the name of the function into the callback */
static TfFunctionEntry FUNCTION_TABLE[]
    = {
          { "print",  tfPrint     },
          { "nop",    tfNop       },
          { "dup",    tfDup       },
          { "+",      tfAdd       },
          { "if",     tfIf        },
          { "ifelse", tfIfelse    },
          { "call",   tfCall      },
          { ">",      tfGreater   },
          { "<",      tfLess      },
          { ">=",     tfGreaterEq },
          { "<=",     tfLessEq    },
          { "==",     tfEquals    },
        //   { "=",      tfAssign    }
      };

#define FUNCTIONS_COUNT (sizeof(FUNCTION_TABLE) / sizeof(TfFunctionEntry))

/** Resolves the function by name, returning NULL if not found. */
TfFunctionEntry *resolveFunctionByName(const char *name) {
    // trivial implementation:
    // - a table contains all the functions, with an asosciated name (string)
    // - resolveFunctionByName finds the function to execute in the table in linear time (O(n))

    for (size_t i = 0; i < FUNCTIONS_COUNT; i++) {
        if (strcmp(name, FUNCTION_TABLE[i].name) == 0) {
            return &(FUNCTION_TABLE[i]);
        }
    }

    return NULL;
}

void executeFunction(TfCtx *ctx, TfWord *fun) {
    assert(fun->type == TF_FUNCTION);

    TfFunctionEntry *fe = resolveFunctionByName(fun->as.str.ptr);
    if (!fe) {
        fprintf(stderr, "Error in executeFunction: invalid symbol encountered: %s\n", fe->name);
        exit(1);
    }

    // calls the callback
    fe->callback(ctx);
}

void execute(TfCtx *ctx, TfWord *program) {
    TfWord *curr = NULL;
    for (size_t i = 0; i < program->as.list.len; i++) {
        curr = program->as.list.words[i];
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