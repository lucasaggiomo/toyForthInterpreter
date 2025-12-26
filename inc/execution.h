#ifndef _EXECUTION_H_
#define _EXECUTION_H_

#include "context.h"

typedef void (*tf_callback)(tf_ctx *);
struct tf_function {
    char *name;
    tf_callback callback;
};
typedef struct tf_function tf_function;

void printWord(tf_word *word, int indentation);

/** Prints the list 'l' in FIFO fashion. */
void printListFIFO(tf_word *l);

/** Prints the stack. */
void printStack(tf_word *stack);

void tfprint(tf_ctx *ctx);
void tfnop(tf_ctx *ctx);

tf_callback getCallback(const char *name);

/** Executes the function whose name is in the word 'fun' (interpreted as a TF_FUNCTION) */
void executeFunction(tf_ctx *ctx, tf_word *fun);

/** Executes each word in the list 'program' with the context 'ctx'. */
void execute(tf_ctx *ctx, tf_word *program);

#endif		// _EXECUTION_H_