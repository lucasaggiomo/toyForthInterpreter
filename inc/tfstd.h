#ifndef _TF_STD_H_
#define _TF_STD_H_

#include "context.h"
#include "execution.h"
#include "word.h"

// convenient macros to define new functions

#define TF_DEF_MONARY(name, identifier, typeMaskOp)                \
    static inline void tf##name(TfCtx *ctx) {                      \
        tfMonaryFun(ctx, #identifier, _tf##name##Fun, typeMaskOp); \
    }

#define TF_DEF_BINARY(name, identifier, typeMaskOp1, typeMaskOp2)                \
    static inline void tf##name(TfCtx *ctx) {                                    \
        tfBinaryFun(ctx, #identifier, _tf##name##Fun, typeMaskOp1, typeMaskOp2); \
    }

#define TF_DEF_TERNARY(name, identifier, typeMaskOp1, typeMaskOp2, typeMaskOp3)                \
    static inline void tf##name(TfCtx *ctx) {                                                  \
        tfTernaryFun(ctx, #identifier, _tf##name##Fun, typeMaskOp1, typeMaskOp2, typeMaskOp3); \
    }

typedef void (*TfMonaryFun)(TfCtx *, TfWord *);
typedef void (*TfBinaryFun)(TfCtx *, TfWord *, TfWord *);
typedef void (*TfTernaryFun)(TfCtx *, TfWord *, TfWord *, TfWord *);
typedef void (*TfNaryFun)(TfCtx *, size_t, TfWord **);
// typedef void (*TfVarargsFun)(TfCtx *, size_t, ...);

void tfMonaryFun(TfCtx *ctx, const char *funName, TfMonaryFun fun, TfType typeMaskOp);
void tfBinaryFun(TfCtx *ctx, const char *funName, TfBinaryFun fun, TfType typeMaskOp1, TfType typeMaskOp2);
void tfTernaryFun(TfCtx *ctx, const char *funName, TfTernaryFun fun, TfType typeMaskOp1, TfType typeMaskOp2, TfType typeMaskOp3);

#define MAX_VARIADIC_ARGS 16
void tfNaryFun(TfCtx *ctx, const char *funName, size_t argc, TfNaryFun fun, TfType *typeMaskOp);

// void tfVarargsFun(TfCtx *ctx, const char *funName, size_t argc, TfVarargsFun fun, ...);

/* =============================== INTERNAL FUNCTIONS =============================== */

void _tfAddFun(TfCtx *ctx, TfWord *op1, TfWord *op2);
void _tfCallFun(TfCtx *ctx, TfWord *program);

/** condition [ifTrue] if */
void _tfIfFun(TfCtx *ctx, TfWord *ifTrue, TfWord *expression);

/** condition [ifTrue] [ifFalse] ifelse */
void _tfIfelseFun(TfCtx *ctx, TfWord *ifFalse, TfWord *ifTrue, TfWord *expression);

void _tfGreaterFun(TfCtx *ctx, TfWord *op1, TfWord *op2);
void _tfLessFun(TfCtx *ctx, TfWord *op1, TfWord *op2);
void _tfGreaterEqFun(TfCtx *ctx, TfWord *op1, TfWord *op2);
void _tfLessEqFun(TfCtx *ctx, TfWord *op1, TfWord *op2);
void _tfEqualsFun(TfCtx *ctx, TfWord *op1, TfWord *op2);

/* =============================== DEFINITIONS =============================== */
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

void printWord(TfWord *word, int indentation);

/** Prints the list 'l' in FIFO fashion. */
void printListFIFO(TfWord *l);

/** Prints the stack. */
void printStack(TfWord *stack);

/** Prints the stack on the screen. */
void tfPrint(TfCtx *ctx);

/** Does nothing. */
void tfNop(TfCtx *ctx);

/** Duplicates the word at the top of the stack. */
void tfDup(TfCtx *ctx);

#endif /* _TF_STD_H_ */