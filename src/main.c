#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: consider creating macros to simplify access to a specific type and prevent ugly things like "(ctx->stack->list.words)[i]"
// TODO: review the parsing of strings and functions
// TODO: delete the assumption that the given program is both syntactically and semantically correct
// TODO: support negative and floating point numbers

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

/* =============================== DATA STRUCTURES =============================== */

enum tftype {
    TF_UNKNOWN,
    TF_NUMBER,
    TF_BOOLEAN,
    TF_STRING,
    TF_LIST,
    TF_FUNCTION
};
typedef enum tftype tftype;

#define MAX_FUN_LEN 32
#define MAX_STR_LEN 256

/**
 * The code to interpret is modeled as a list of 'words'.
 * Whenever an operand is encountered on a command, it is pushed on the stack.
 * A WORD can be:
 *  - a TF_NUMBER (only integer for now). A character is interpreted as an integer whose value corresponds to the ASCII value of the character.
 *  - a TF_STRING (series of characters of a fixed length) [terminated by null character (?)]
 *  - a TF_LIST, defined as an ordered sequence of words. Note that a COMMAND is modeled as a list.
 *  - a TF_FUNCTION ('+' in the last example). It is just a symbol that represents something to execute.
 *
 * A word is composed by:
 *  - an HEADER, that contains all the relevant metadata regarding the word
 *  - a PAYLOAD, containing the word's data
 */
struct tfword {
    // HEADER
    int refcount;
    tftype type;

    // PAYLOAD
    union {
        // TF_NUMBER | TF_BOOLEAN
        int num;

        // TF_STRING | TF_FUNCTION
        struct {
            size_t len;
            char *ptr;
        } str;

        // TF_LIST: modeled as an array of type word
        struct {
            size_t len;
            size_t capacity;
            struct tfword **words;     // array dinamico di puntatori a tfword
        } list;
    };
};
typedef struct tfword tfword;

struct tfparser {
    char *program;     // the program to parse
    char *next;        // next 'token' to parse
};
typedef struct tfparser tfparser;

struct tfctx {
    tfword *stack;     // a stack is a pointer to a tfword of type TF_LIST
};
typedef struct tfctx tfctx;

/* =============================== ALLOCATION WRAPPERS =============================== */

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("Error in malloc");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *tmp = realloc(ptr, size);
    if (tmp == NULL) {
        perror("Error in realloc");
        exit(1);
    }
    return tmp;
}

/* =============================== CREATIONAL PROCEDURES =============================== */

tfword *createWord(tftype type) {
    tfword *word = xmalloc(sizeof(*word));
    word->type = type;
    word->refcount = 1;
    return word;
}

void freeWord(tfword *word) {
    if (word->type == TF_FUNCTION || word->type == TF_STRING) {
        free(word->str.ptr);
        word->str.ptr = NULL;
        word->str.len = 0;
    }
    free(word);
}

void retainWord(tfword *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried retaining word with refcount = 0");
        exit(1);
    }
    (word->refcount)++;
}

void releaseWord(tfword *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried releasing word with refcount = 0");
        exit(1);
    }
    (word->refcount)--;

    if (word->refcount == 0) freeWord(word);
}

tfword *createNumber(int num) {
    tfword *word = createWord(TF_NUMBER);
    word->num = num;
    return word;
}

tfword *createBoolean(int value) {
    tfword *word = createNumber(value);
    word->type = TF_BOOLEAN;
    return word;
}

tfword *createString(char *string, size_t length) {
    tfword *word = createWord(TF_STRING);
    word->str.len = length;
    word->str.ptr = string;
    return word;
}

tfword *createFunction(char *funName, size_t length) {
    tfword *word = createString(funName, length);
    word->type = TF_FUNCTION;
    return word;
}

tfword *createList() {
    tfword *word = createWord(TF_LIST);
    word->list.len = 0;
    word->list.capacity = 0;
    word->list.words = NULL;
    return word;
}

/** END OF CREATIONAL PROCEDURES */

/**
 * Pushes 'word' on the end of the list 'l', reallocating the necessary space if needed.
 * It retains the word.
 */
void listPush(tfword *l, tfword *word) {
    // checks if there's enough space, otherwise it doubles the capacity
    if (l->list.len == l->list.capacity) {
        size_t newCapacity = (l->list.capacity == 0) ? 8 : l->list.capacity * 2;     // TODO: check overflow
        l->list.words = xrealloc(l->list.words, newCapacity * sizeof(*(l->list.words)));
        l->list.capacity = newCapacity;
    }

    l->list.words[l->list.len] = word;
    (l->list.len)++;

    retainWord(word);
}

/**
 * Pops the word at the and of the list 'l', or NULL if list is empty.
 * Note that the caller has to release the word at the end of usage.
 */
tfword *listPop(tfword *l) {
    if (l->list.len == 0)
        return NULL;

    (l->list.len)--;
    return l->list.words[l->list.len];
}

void releaseList(tfword *l) {
    // pops every word in the list
    while (l->list.len > 0) {
        releaseWord(listPop(l));
    }

    free(l->list.words);
    l->list.words = NULL;
    l->list.capacity = 0;
}

void initCtx(tfctx *ctx) {
    // creates the stack
    ctx->stack = createList();
}

void deinitCtx(tfctx *ctx) {
    // releases the content of the stack and the stack itself
    releaseList(ctx->stack);
    releaseWord(ctx->stack);
}

/** Pushes the *tfword 'word' on the stack.  */
void push(tfctx *ctx, tfword *word) {
    listPush(ctx->stack, word);
}

/** Pops the tfword on the top on the stack. */
tfword *pop(tfctx *ctx) {
    return listPop(ctx->stack);
}

/** Compiles a single word from the parser. Returns NULL if parsing ended or if end of list ']' is encountered */
tfword *parseNextWord(tfparser *parser) {
    if (parser->next == NULL || *(parser->next) == '\0') return NULL;

    /* Detects type of word encountered:
     *  - letter (between 'A' and 'Z' or between 'a' and 'z') => FUNCTION
     *  - digit (between '0' and '9') => NUMBER (assuming only positive and integer numbers)
     *  - character '"' => STRING
     *  - character '[' => LIST
     */
    tfword *word;
    if ((parser->next[0] >= 'A' && parser->next[0] <= 'Z') || (parser->next[0] >= 'a' && parser->next[0] <= 'z')) {

        // FUNCTION
        char *funName = xmalloc(MAX_FUN_LEN * sizeof(*funName));
        int n = sscanf(parser->next, "%s", funName);
        if (n <= 0) {
            perror("Errore in sscanf");
            exit(1);
        }
        word = createFunction(funName, strlen(funName));

    } else if (parser->next[0] >= '0' && parser->next[0] <= '9') {

        // NUMBER
        int value;
        int n = sscanf(parser->next, "%d", &value);
        if (n <= 0) {
            perror("Errore in sscanf");
            exit(1);
        }
        word = createNumber(value);

    } else if (parser->next[0] == '\"') {

        // TODO: what if a string contains a space? Here is assumed string doesn't contain any space

        // STRING
        char *ptr = xmalloc(MAX_STR_LEN * sizeof(*ptr));
        int n = sscanf(parser->next, "\"%s\"", ptr);
        if (n <= 0) {
            perror("Error in sscanf");
            exit(1);
        }
        word = createString(ptr, strlen(ptr));

    } else if (parser->next[0] == '[') {

        // LIST
        word = createList();

        // populates the list by parsing each word contained in the list until character ']'
        tfword *curr = NULL;
        while (1) {
            parser->next = strtok(NULL, " ");
            curr = parseNextWord(parser);
            if (curr == NULL) break;

            listPush(word, curr);
            releaseWord(curr);
        };

    } else if (parser->next[0] == ']') {
        return NULL;
    } else {
        fprintf(stderr, "Error in parsing word at position %ld\n", parser->next - parser->program);
        exit(1);
    }

    return word;
}
/**
 * Compiles 'program' into a list.
 * SIMPLIFIED VERSION: assumes that the words are separated by a single space (except for a list, composed by '[ {words separated by space} ]'
 */
tfword *compile(char *program, size_t length) {
    (void)length;

    tfword *list = createList();

    /** Parsing of program... */

    // creates a parser and starts parsing every word of the program
    tfparser parser = {
        .program = program,
        .next = program
    };

    tfword *curr = parseNextWord(&parser);
    parser.next = strtok(program, " ");
    do {
        listPush(list, curr);
        releaseWord(curr);

        curr = parseNextWord(&parser);
        parser.next = strtok(NULL, " ");
    } while (curr != NULL);

    return list;
}

static char *tftypeName[] = {
    "UNKNOWN",
    "NUMBER",
    "BOOLEAN",
    "STRING",
    "LIST",
    "FUNCTION"
};

typedef void (*tfcallback)(tfctx *);
struct tffunction {
    char *name;
    tfcallback callback;
};
typedef struct tffunction tffunction;

/** Prints the list 'l'. */
void printList(tfword *l, int indentation) {
    for (int i = l->list.len - 1; i >= 0; i--) {
        // trivial implementation
        tfword *curr = l->list.words[i];

        for (int j = 0; j < indentation; j++) printf("\t");

        printf("[%s] ", tftypeName[curr->type]);
        switch (curr->type) {
            case TF_NUMBER:
                printf("%d\n", curr->num);
                break;
            case TF_BOOLEAN:
                printf("%s\n", curr->num == 0 ? "false" : "true");
                break;
            case TF_STRING:
                printf("\"%s\"\n", curr->str.ptr);
                break;
            case TF_FUNCTION:
                printf("%s\n", curr->str.ptr);
                break;
            case TF_LIST:
                printf("[\n");
                printList(curr, indentation + 1);     // recursively print the list
                for (int j = 0; j < indentation; j++) printf("\t");
                printf("]\n");
                break;
            default:
                fprintf(stderr, "Invalid word type encountered: %d\n", curr->type);
                exit(1);
        }
    }
}
void tfprint(tfctx *ctx) {
    printf("=== STACK ===\n");
    printList(ctx->stack, 0);
    printf("=============\n");
}

void tfnop(tfctx *ctx) {
    (void)ctx;
}

static tffunction functionTable[]
    = {
          { "print", tfprint },
          { "nop", tfnop }
          // { "dup", tfdup },
          // { "+", tfadd }
      };

#define FUNCTIONS_COUNT (sizeof(functionTable) / sizeof(tffunction))

void executeFunction(tfctx *ctx, tfword *fun) {
    int found = 0;
    for (size_t i = 0; i < FUNCTIONS_COUNT; i++) {
        if (strcmp(fun->str.ptr, functionTable[i].name) == 0) {
            functionTable[i].callback(ctx);
            found = 1;
            break;
        }
    }
    if (found == 0) {
        fprintf(stderr, "Invalid symbol encountered: %s\n", fun->str.ptr);
        exit(1);
    }
}

/** Executes each word in the list 'program' with the context 'ctx'. */
void execute(tfctx *ctx, tfword *program) {
    tfword *curr = NULL;
    for (size_t i = 0; i < program->list.len; i++) {
        curr = program->list.words[i];
        switch (curr->type) {
            case TF_NUMBER:
            case TF_BOOLEAN:
            case TF_STRING:
            case TF_LIST:
                // pushes the word on the stack
                push(ctx, curr);
                break;
            case TF_FUNCTION:
                // executes the function with the associated name
                executeFunction(ctx, curr);
                break;
            default:
                fprintf(stderr, "Invalid word type encountered: %d\n", curr->type);
                exit(1);
        }
    }
}

/** Compiles 'program' into a list - STUB VERSION FOR TESTING */
tfword *compilestub(char *program, size_t length) {
    (void)program;     // Ignora i parametri per ora
    (void)length;

    tfword *list = createList();

    tfword *num = createNumber(42);
    listPush(list, num);

    // Push booleano true
    tfword *bool = createBoolean(1);
    listPush(list, bool);
    releaseWord(bool);

    // Push stringa "hello"
    tfword *string = createString("hello", 5);
    listPush(list, string);
    releaseWord(string);

    tfword *list2 = createList();
    listPush(list, list2);
    releaseWord(list2);

    tfword *num2 = createNumber(12);
    listPush(list2, num2);
    releaseWord(num2);

    tfword *string2 = createString("giovanni", 5);
    listPush(list2, string2);
    releaseWord(string2);

    // Push funzione "print"
    tfword *print = createFunction("print", 5);
    listPush(list, print);
    releaseWord(print);

    return list;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:\n>> %s <command>\n", argv[0]);
        return 1;
    }

    // single context for now
    tfctx context;

    initCtx(&context);

    // parsing of the parameters
    printf("I read \'%s\'\n", argv[1]);

    char *program = argv[1];
    size_t len = strlen(program);

    printf("Program to interpret:\n>> \'%s\'\n", program);

    printf("\n\n==============================================\nInterpreting...\n");

    tfword *compiledProg = compile(program, len);

    printf("\n\n==============================================\nExecuting...\n");

    execute(&context, compiledProg);

    printf("\n\n==============================================\nProgram execution terminated, this is the stack:\n");
    tfprint(&context);

    releaseList(compiledProg);
    releaseWord(compiledProg);

    deinitCtx(&context);

    return 0;
}
