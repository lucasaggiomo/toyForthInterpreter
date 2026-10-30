#include "execution.h"

#include "context.h"
#include "da.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tfRequireType(
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
            getTypeName(op->type)
        );
    }
}

/** Resolves the function by name from the context, returning NULL if not found. */
TfFunctionEntry *resolveFunctionByName(TfCtx *ctx, StringView *sv) {
    // trivial implementation:
    // - a table contains all the functions, with an asosciated name (string)
    // - resolveFunctionByName finds the function to execute in the table in linear time (O(n))

    for (size_t i = 0; i < ctx->funTable->count; i++) {
        TfFunctionEntry *entry = da_get(ctx->funTable, i);
        if (svCompare(sv, &entry->name) == 0)
            return entry;
    }

    return NULL;
}

void executeTheExecutable(TfCtx *ctx, TfFunctionEntry *fe) {
    switch (fe->kind) {
        case TF_EXECUTABLE_CALLBACK:
            // executes the C callback associated to the function entry
            fe->as.callback(ctx);
            break;

        case TF_EXECUTABLE_NATIVE:
            // recursively executes the TF_FUNCTION (i.e. with the same 'execute' used to execute the current program itself)
            execute(ctx, fe->as.native);
            break;

        default:
            fprintf(stderr, "Error in executeTheExecutable: invalid function entry kind encountered: %d\n", fe->kind);
            exit(1);
    }
}

void executeFunction(TfCtx *ctx, TfWord *fun) {
    assert(fun->type == TF_FUNCTION);

    StringView sv = svFromSb(&fun->as.str);

    // resolves the function
    TfFunctionEntry *fe = resolveFunctionByName(ctx, &sv);
    if (!fe) {
        fprintf(stderr, "Error in executeFunction: invalid symbol encountered: " SV_FMT "\n", SV_ARGS(&fe->name));
        exit(1);
    }

    // executes the function
    executeTheExecutable(ctx, fe);
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