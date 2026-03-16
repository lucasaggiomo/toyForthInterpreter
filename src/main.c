#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "execution.h"
#include "utils.h"

/**
 * TODO: consider creating macros to simplify access to a specific type and prevent ugly things like "(ctx->stack->list.words)[i]"
 * TODO: delete the assumption that the given program is both syntactically and semantically correct
 * TODO: support floating point numbers
 * TODO: refactor functions, implementing a 'polymorphic' behaviour of the words, in order to remove some of the switch cases (ex: pointer to functions based on type)
 * TODO: refactor program execution and functions in another file: create an abstraction of the "executor" that executes the program. The main.c has to do nothing.
 * TODO: error handling
 * FIXME: the operations that work on a list interpret it as a "stack" (LIFO) [see listPush, listPop, listPeek]. This can be counter-intuitive
 */

/**
 * Program example #1:
 * > 1 2 +
 *
 * The words '1' and '2' are inserted on the stack from left to right:
 *
 * Stack before executing the operation '+':
 *  +   +
 *  | 2 |
 *  +---+
 *  | 1 |
 *  +---+
 *
 * The word '+' determines the execution of the sum function on the last two added word present on the stack.
 * So it pops two word and pushes the result of the sum on the stack:
 *
 * Stack after executing the operation '+':
 *  +   +
 *  | 3 |
 *  +---+
 *
 * Program example #2:
 * > 3 [dup *] [dup +] [1 2 >] ifelse
 *
 * Stack before parsing and executing 'ifelse'
 *  +         +
 *  | [1 2 >] |
 *  +---------+
 *  | [dup +] |
 *  +---------+
 *  | [dup *] |
 *  +---------+
 *  | 3       |
 *  +---------+
 *
 * Where '[dup *]', '[dup +]' and '[1 2 >]' can be seen as "programs" that haven't been executed yet.
 * The symbol / function 'ifelse' pops and execute the sub-program '[1 2 >]', and checks its return boolean value (in this case 'true').
 * Based on this boolean value, it executes either '[dup +]' or '[dup *]', when the condition is true or false respectively.
 *
 * In this example it is executed [dup *] on the remaining stack, containing '42':
 *  +   +
 *  | 3 |
 *  +---+
 *
 * - 'dup' duplicates the last element on the stack (i.e. pops a word and pushes it twice / reads a word and pushes it once);
 *  +   +
 *  | 3 |
 *  +---+
 *  | 3 |
 *  +---+
 * - '*' pops two words, interpreting them as a number, and pushes the result of their moltiplication
 *  +   +
 *  | 9 |
 *  +---+
 *
 * Therefore the program "dup *" calculates the square of the number on the stack.

 */

#define PRINTF_HEADER(fmt, ...)            \
    do {                                   \
        printf("\n==================== "); \
        printf(fmt, ##__VA_ARGS__);        \
        printf(" ====================\n"); \
    } while (0)

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:\n>> %s <command>\n", argv[0]);
        return 1;
    }

    // parsing of the parameters

    PRINTF_HEADER("Reading file \'%s\'...", argv[1]);
    char *program = NULL;
    size_t len = readSourceCode(argv[1], &program);

    // single context for now
    tf_ctx context;

    initCtx(&context);

    printf("Program to interpret:\n>> \'%s\'\n", program);

    PRINTF_HEADER("Interpreting...");

    tf_word *compiledProg = compile(program, len);

    printf(">> The compiled program looks like this:\n");
    printListFIFO(compiledProg);

    PRINTF_HEADER("Executing...");

    execute(&context, compiledProg);

    PRINTF_HEADER("Terminating...");
    printf("\nProgram execution terminated, this is the stack:\n");
    tfprint(&context);

    releaseWord(compiledProg);

    deinitCtx(&context);

    free(program);

    return 0;
}
