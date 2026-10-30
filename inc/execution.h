#ifndef _TF_EXECUTION_H_
#define _TF_EXECUTION_H_

#include "strview.h"
#include "word.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct TfCtx TfCtx;

// aborts the execution due to a semantic error of the ToyForth program
#define ABORT_EXECUTION(fmt, ...)               \
    do {                                        \
        fprintf(stderr, "EXECUTION FAILURE: "); \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        fprintf(stderr, "\n");                  \
        exit(EXIT_FAILURE);                     \
    } while (0)

// an assert-like instruction that is convenient to reduce boilerplate code in functions
#define TF_REQUIRE(expr, fmt, ...)               \
    do {                                         \
        if (!(expr))                             \
            ABORT_EXECUTION(fmt, ##__VA_ARGS__); \
    } while (0)

void tfRequireType(const char *funName, TfWord *op, TfType mask, size_t num);

typedef enum TfExecutableKind {
    TF_EXECUTABLE_CALLBACK,
    TF_EXECUTABLE_NATIVE
} TfExecutableKind;

typedef union TfExecutable {

    /** C callback associated to TfFunction (for ToyForth std library functions). */
    void (*callback)(TfCtx *);

    /** Native TfFunction (TfWord of type TF_FUNCTION) (for user-defined functions). */
    TfWord *native;

} TfExecutable;

typedef struct TfFunctionEntry {
    // HEADER
    StringView name;
    TfExecutableKind kind;

    // PAYLOAD
    TfExecutable as;
} TfFunctionEntry;

typedef struct TfFunctionTable {
    TfFunctionEntry **items;
    size_t count;
    size_t capacity;
} TfFunctionTable;

/** Executes the function whose name is in the word 'fun' (interpreted as a TF_FUNCTION) */
void executeFunction(TfCtx *ctx, TfWord *fun);

/** Executes each word in the list 'program' with the context 'ctx'. */
void execute(TfCtx *ctx, TfWord *program);

#endif     // _TF_EXECUTION_H_