#ifndef _EXECUTION_H_
#define _EXECUTION_H_

#include "context.h"

typedef void (*TfCallback)(TfCtx *);
typedef struct TfFunctionEntry {
    char *name;
    TfCallback callback;
} TfFunctionEntry;

void printWord(TfWord *word, int indentation);

/** Prints the list 'l' in FIFO fashion. */
void printListFIFO(TfWord *l);

/** Prints the stack. */
void printStack(TfWord *stack);

void tfPrint(TfCtx *ctx);
void tfNop(TfCtx *ctx);

/** Executes the function whose name is in the word 'fun' (interpreted as a TF_FUNCTION) */
void executeFunction(TfCtx *ctx, TfWord *fun);

/** Executes each word in the list 'program' with the context 'ctx'. */
void execute(TfCtx *ctx, TfWord *program);

#endif		// _EXECUTION_H_