#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "parser.h"
#include "context.h"

// TODO: consider creating macros to simplify access to a specific type and prevent ugly things like "(ctx->stack->list.words)[i]"
// TODO: delete the assumption that the given program is both syntactically and semantically correct
// TODO: support floating point numbers
// TODO: refactor functions, implementing a 'polymorphic' behaviour of the words, in order to remove some of the switch cases (ex: pointer to functions based on type)
// TODO: refactor program execution and functions in another file: create an abstraction of the "executor" that executes the program. The main.c has to do nothing.
// TODO: error handling

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
 * > 3 [dup +] [dup *] [1 2 >] ifelse
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

/**
 * Compiles 'program' into a list.
 */
tf_word *compile(char *program, size_t length) {
    (void)length;

    // creates a lexer and gets the next token. Then the token is passed to the parser, that creates the data structure of words modeling the program

    tf_lexer lexer = {
        .program = program,
        .next = program
    };

    tf_word *list = parse(&lexer);

    return list;
}

/* ============================== FUNCIONS AND EXECUTION =============================== */

/*
static char *tf_typeName[] = {
    "UNKNOWN",
    "NUMBER",
    "BOOLEAN",
    "STRING",
    "LIST",
    "FUNCTION"
}
*/

typedef void (*tf_callback)(tf_ctx *);
struct tf_function {
    char *name;
    tf_callback callback;
};
typedef struct tf_function tf_function;

void printWord(tf_word *word, int indentation) {
    // for (int j = 0; j < indentation; j++) printf("\t");

    //   printf("[%s] ", tf_typeName[word->type]);
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
            printf("%s ", word->str.ptr);
            break;
        case TF_LIST:
            printf("[ ");
            for (size_t i = 0; i < word->list.len; i++) {
                printWord(word->list.words[i], indentation + 1);     // recursively prints the list
            }
            // for (int j = 0; j < indentation; j++) printf("\t");
            printf("] ");
            break;
        default:
            fprintf(stderr, "Error in printWord: invalid word type encountered: %d\n", word->type);
            exit(1);
    }
}
/** Prints the list 'l'. */
void printList(tf_word *l) {
    for (size_t i = 0; i < l->list.len; i++) {
        printWord(l->list.words[i], 0);
    }
}

/** Prints the stack. */
void printStack(tf_word *stack) {
    for (size_t i = 1; i <= stack->list.len; i++) {
        printWord(stack->list.words[stack->list.len - i], 0);
    }
}

void tfprint(tf_ctx *ctx) {
    printf("=== STACK ===\n");
    printStack(ctx->stack);
    printf("\n=============\n");
}

void tfnop(tf_ctx *ctx) {
    (void)ctx;
}

static tf_function functionTable[]
    = {
          { "print", tfprint },
          { "nop", tfnop }
          // { "dup", tfdup },
          // { "+", tfadd }
      };

#define FUNCTIONS_COUNT (sizeof(functionTable) / sizeof(tf_function))

void executeFunction(tf_ctx *ctx, tf_word *fun) {
    int found = 0;
    for (size_t i = 0; i < FUNCTIONS_COUNT; i++) {
        if (strcmp(fun->str.ptr, functionTable[i].name) == 0) {
            functionTable[i].callback(ctx);
            found = 1;
            break;
        }
    }
    if (found == 0) {
        fprintf(stderr, "Error in executeFunction: invalid symbol encountered: %s\n", fun->str.ptr);
        exit(1);
    }
}

/** Executes each word in the list 'program' with the context 'ctx'. */
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

size_t readSourceCode(const char *filepath, char **program) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error in opening file %s: ", filepath);
        perror("");     // super ugly: idgas
        exit(1);
    }

    // calculate size of source code
    fseek(file, 0, SEEK_END);

    size_t len = ftell(file);
    *program = xmalloc((len + 1) * sizeof(**program));

    // reset cursor
    fseek(file, 0, SEEK_SET);

    // read all the content of file (len elements of size '1' each)
    size_t read = fread(*program, 1, len, file);
    if (read != len) {
        fprintf(stderr, "Error reading file: expected %zu bytes, got %zu\n", len, read);
        exit(1);
    }

    (*program)[len] = '\0';

    fclose(file);

    return len;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:\n>> %s <command>\n", argv[0]);
        return 1;
    }

    // parsing of the parameters
    printf("I read file \'%s\'\n", argv[1]);
    char *program = NULL;
    size_t len = readSourceCode(argv[1], &program);

    // single context for now
    tf_ctx context;

    initCtx(&context);

    printf("Program to interpret:\n>> \'%s\'\n", program);

    printf("\n\n==============================================\nInterpreting...\n");

    tf_word *compiledProg = compile(program, len);

    printf(">> The compiled program looks like this:\n");
    printList(compiledProg);

    printf("\n\n==============================================\nExecuting...\n");

    execute(&context, compiledProg);

    printf("\n\n==============================================\nProgram execution terminated, this is the stack:\n");
    tfprint(&context);

    releaseWord(compiledProg);

    deinitCtx(&context);

    free(program);

    return 0;
}
