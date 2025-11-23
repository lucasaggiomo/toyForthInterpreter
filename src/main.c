#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: consider creating macros to simplify access to a specific type and prevent ugly things like "(ctx->stack->list.words)[i]"
// TODO: delete the assumption that the given program is both syntactically and semantically correct
// TODO: support floating point numbers
// TODO: implement a 'polymorphic' behaviour of the words, in order to remove some of the switch cases (ex: pointer to functions based on type)
// TODO: refactor code in multiple files
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

/* =============================== DATA STRUCTURES =============================== */

enum tf_type {
    TF_UNKNOWN,
    TF_NUMBER,
    TF_BOOLEAN,
    TF_STRING,
    TF_LIST,
    TF_FUNCTION
};
typedef enum tf_type tf_type;

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
struct tf_word {
    // HEADER
    int refcount;
    tf_type type;

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
            struct tf_word **words;     // array dinamico di puntatori a tf_word
        } list;
    };
};
typedef struct tf_word tf_word;

struct tf_ctx {
    tf_word *stack;     // a stack is a pointer to a tf_word of type TF_LIST
};
typedef struct tf_ctx tf_ctx;

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

tf_word *createWord(tf_type type) {
    tf_word *word = xmalloc(sizeof(*word));
    word->type = type;
    word->refcount = 1;
    return word;
}

void releaseWord(tf_word *word);     // temporary, needs refactoring

void freeWord(tf_word *word) {
    switch (word->type) {
        case TF_STRING:
        case TF_FUNCTION:
            free(word->str.ptr);
            break;
        case TF_LIST:
            // release every word contained in the list
            for (size_t i = 0; i < word->list.len; i++) {
                releaseWord(word->list.words[i]);
            }
            free(word->list.words);
            break;
        // to suppress warning -Wswitch
        case TF_NUMBER:
        case TF_BOOLEAN:
        case TF_UNKNOWN:
        default:
            break;
    }
    free(word);
}

void retainWord(tf_word *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried retaining word with refcount = 0");
        exit(1);
    }
    (word->refcount)++;
}

void releaseWord(tf_word *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried releasing word with refcount = 0");
        exit(1);
    }
    (word->refcount)--;

    if (word->refcount == 0) freeWord(word);
}

tf_word *createNumber(int num) {
    tf_word *word = createWord(TF_NUMBER);
    word->num = num;
    return word;
}

tf_word *createBoolean(int value) {
    tf_word *word = createNumber(value);
    word->type = TF_BOOLEAN;
    return word;
}

/**
 * Creates a new tf_word of type TF_STRING, copying exactly 'length' bytes from 'string' in the new tf_word created.
 * Therefore the input string doesn't need to be NULL terminated, as the NULL terminator doesn't stop the copy.
 * The created string is NULL terminated.
 */
tf_word *createString(char *string, size_t length) {
    tf_word *word = createWord(TF_STRING);
    word->str.ptr = xmalloc((length + 1) * sizeof(*(word->str.ptr)));
    memcpy(word->str.ptr, string, length);
    word->str.ptr[length] = '\0';
    word->str.len = length;
    return word;
}

tf_word *createFunction(char *funName, size_t length) {
    tf_word *word = createString(funName, length);
    word->type = TF_FUNCTION;
    return word;
}

tf_word *createList() {
    tf_word *word = createWord(TF_LIST);
    word->list.len = 0;
    word->list.capacity = 0;
    word->list.words = NULL;
    return word;
}

/** END OF CREATIONAL PROCEDURES */

/**
 * Pushes 'word' on the end of the list 'l', reallocating the necessary space if needed.
 * It doesn't retain the word, assuming the caller wants to release it. Otherwise the caller needs to retain the word after listPush.
 */
void listPush(tf_word *l, tf_word *word) {
    // checks if there's enough space, otherwise it doubles the capacity
    if (l->list.len == l->list.capacity) {
        size_t newCapacity = (l->list.capacity == 0) ? 8 : l->list.capacity * 2;     // TODO: check overflow
        l->list.words = xrealloc(l->list.words, newCapacity * sizeof(*(l->list.words)));
        l->list.capacity = newCapacity;
    }

    l->list.words[l->list.len] = word;
    (l->list.len)++;
}

/**
 * Pops the word at the and of the list 'l', or NULL if list is empty.
 * Note that the caller has to release the word at the end of usage.
 */
tf_word *listPop(tf_word *l) {
    if (l->list.len == 0)
        return NULL;

    (l->list.len)--;
    return l->list.words[l->list.len];
}

void initCtx(tf_ctx *ctx) {
    ctx->stack = createList();
}

void deinitCtx(tf_ctx *ctx) {
    releaseWord(ctx->stack);
}

/** Pushes the *tf_word 'word' on the stack.  */
void push(tf_ctx *ctx, tf_word *word) {
    listPush(ctx->stack, word);
}

/** Pops the tf_word on the top on the stack. */
tf_word *pop(tf_ctx *ctx) {
    return listPop(ctx->stack);
}

/* ======================================= PARSING ===================================== */

/** source code ==[LEXER]=> stream of tokens ==[PARSER]=> data structure modeling the program ==[EXECUTOR]=> execution of the program. */

/** Defines the types of token that a lexer can create. */
enum tf_tokentype {
    TF_TOK_NUMBER,
    TF_TOK_STRING,
    TF_TOK_IDENTIFIER,     // function, variable (in the future)
    TF_TOK_LBRACKET,
    TF_TOK_RBRACKET,
    TF_TOK_COMMENT,
    TF_TOK_EOF,
    TF_TOK_ERROR
};
typedef enum tf_tokentype tf_toktype;

/** a token is defined as the lexical unit found in the source code */
struct tf_token {
    tf_toktype type;
    char *str;      // content of the token
    size_t len;     // length of str
    //    size_t line;
    //    size_t pos;
};
typedef struct tf_token tf_token;

/** the lexer finds the tokens in the source code */
struct tf_lexer {
    char *program;
    char *next;
    //    size_t line;
    //    size_t col;
};
typedef struct tf_lexer tf_lexer;

/** Advances lexer->next to the next non-space character (a character is defined as a space iff isspace(c) == 0). */
void skipSpaces(tf_lexer *lexer) {
    while (isspace(lexer->next[0])) {
        (lexer->next)++;
    }
}

int isValidSymbol(int c) {
    static char *validSymbols = "+-*/%><=";
    return c != '\0' && (isalpha(c) || strchr(validSymbols, c) != NULL);
}

/** Finds the next token of the source code from lexer->next, and updates this pointer. */
tf_token findNextToken(tf_lexer *lexer) {
    if (lexer->next == NULL)
        return (tf_token) { TF_TOK_EOF, lexer->next, 0 };

    // skips starting spaces
    skipSpaces(lexer);

#ifdef DEBUG
    printf("[DEBUG]: lexer->next = '%c' (offset %ld)\n",
           lexer->next[0], lexer->next - lexer->program);
#endif

    if (lexer->next[0] == '\0')
        return (tf_token) { TF_TOK_EOF, lexer->next, 0 };

    /* Detects type of token encountered:
     *  - letter (between 'A' and 'Z' or between 'a' and 'z') => TF_TOK_IDENTIFIER
     *  - digit (between '0' and '9') or '-' => TF_TOK_NUMBER
     *  - character '"' => TF_TOK_STRING
     *  - character '[' => TF_TOK_LBRACKET
     *  - character ']' => TF_TOK_RBRACKET
     */

    char *start = lexer->next;
    size_t len = 0;
    tf_toktype type = TF_TOK_ERROR;
    if (isdigit(lexer->next[0]) || (lexer->next[0] == '-' && isdigit(lexer->next[1]))) {

        type = TF_TOK_NUMBER;
        if (lexer->next[0] == '-')
            (lexer->next)++;
        while (isdigit(lexer->next[0])) {
            (lexer->next)++;
        }
        len = lexer->next - start;

    } else if (isValidSymbol(lexer->next[0])) {

        type = TF_TOK_IDENTIFIER;
        while (isValidSymbol(lexer->next[0])) {
            (lexer->next)++;
        }
        len = lexer->next - start;

    } else if (lexer->next[0] == '\"') {

        type = TF_TOK_STRING;
        start = ++(lexer->next);     // increments lexer->next to skip starting '\"'. Then updates start
        while (lexer->next[0] != '\"' && lexer->next[0] != '\0') {
            (lexer->next)++;
        }

        if (lexer->next[0] == '\0') {
            // string ended without terminal quotes '\"' character
            fprintf(stderr, "Error in findNextToken: unterminated string (reached EOF)\n");
            exit(1);
        }

        len = lexer->next - start;
        (lexer->next)++;     // skips last '\"'

    } else if (lexer->next[0] == '[') {

        type = TF_TOK_LBRACKET;
        (lexer->next)++;
        len = lexer->next - start;

    } else if (lexer->next[0] == ']') {

        type = TF_TOK_RBRACKET;
        (lexer->next)++;
        len = lexer->next - start;

    } else if (lexer->next[0] == '#') {

        type = TF_TOK_COMMENT;
        start = ++(lexer->next);     // increments lexer->next to skip starting '#'. Then updates start

        // searches for newline character
        while(lexer->next[0] != '\n'){
            (lexer->next)++;
        }
        (lexer->next)++;             // skips '\n'
        len = lexer->next - start;

    } else {
        fprintf(stderr, "Error in findNextToken: unrecognized starting character at position %ld: \'%c\'.\nNext 32 characters between quotes:\n\'%32s\'\n",
                lexer->next - lexer->program, lexer->next[0], lexer->next);
        exit(1);
    }

    return (tf_token) { type, start, len };
}

struct tf_parser {
    tf_lexer *lexer;
    int bracket_depth;
};
typedef struct tf_parser tf_parser;

/**
 * Parses a number word from 'token'. Assumes that token->type == TK_TOK_NUMBER.
 * Returns the tf_word* containing the parsed number.
 */
tf_word *parseNumber(tf_token *token) {
    char *end = NULL;

    // only int for now
    int num = (int)strtol(token->str, &end, 0);
    if (token->len != (size_t)(end - token->str)) {     // strtol guarantees that end >= token->str
        fprintf(stderr, "Error in parseNumber: expected a number of length %zu digits (sign included if negative), but instead found one of length %zu\n",
                token->len, end - token->str);
        exit(1);
    }
    return createNumber(num);
}

/**
 * Parses a symbol word from 'token'. Assumes that token->type == TF_TOK_IDENTIFIER.
 * Returns the tf_word* containing the parsed symbol name.
 */
tf_word *parseSymbol(tf_token *token) {
    return createFunction(token->str, token->len);
}

/**
 * Parses a string word from 'token'. Assumes that token->type == TF_TOK_STRING.
 * Returns the tf_word* containing the parsed string.
 */
tf_word *parseString(tf_token *token) {
    return createString(token->str, token->len);
}

/** Given a token and a lexer, creates an associated tf_word object. Returns NULL if token type is TF_TOK_EOF or TF_TOK_RBRACKET. */
tf_word *parseToken(tf_parser *p, tf_token *token) {
    tf_word *word = NULL;

    switch (token->type) {
        case TF_TOK_NUMBER:
            // TF_NUMBER
            word = parseNumber(token);
            break;
        case TF_TOK_STRING:
            // TF_STRING
            word = parseString(token);
            break;
        case TF_TOK_IDENTIFIER:
            // TF_FUNCTION
            word = parseSymbol(token);
            break;
        case TF_TOK_LBRACKET:
            // start of TF_LIST
            word = createList();

            (p->bracket_depth)++;

            // populates the list by getting new tokens until a TF_TOK_RBRACKET is found
            tf_token next;
            while ((next = findNextToken(p->lexer)).type != TF_TOK_RBRACKET) {
                if(next.type == TF_TOK_EOF){
                    fprintf(stderr, "Error in parseToken: unclosed bracket (reached EOF)\n");
                    exit(1);
                }
                tf_word *element = parseToken(p, &next);
                if(element != NULL){
                    listPush(word, element);
                }
            }

            (p->bracket_depth)--;
            break;
        case TF_TOK_RBRACKET:
            // end of TF_LIST
            break;
        case TF_TOK_COMMENT:
#ifdef DEBUG
            printf("[DEBUG] Encountered comment \'%.*s\'\n", (int)token->len, token->str);
#endif
            break;
        case TF_TOK_EOF:
            break;
        case TF_TOK_ERROR:
            fprintf(stderr, "Error in parseToken: error token found at position %ld. Next 32 characters between quotes:\n\'%32s\'\n",
                    p->lexer->next - p->lexer->program, p->lexer->next);
            exit(1);
        default:
            fprintf(stderr, "Error in parseToken: unrecognized token found at position %ld. Next 32 characters between quotes:\n\'%32s\'\n",
                    p->lexer->next - p->lexer->program, p->lexer->next);
            exit(1);
    }

    return word;
}

/** Parses the program from a given lexer and creates a list containing the words that model the program. */
tf_word *parse(tf_lexer *lexer) {
    tf_word *list = createList();

    tf_token token;
    tf_parser p = {.lexer = lexer, .bracket_depth = 0};
    while ((token = findNextToken(lexer)).type != TF_TOK_EOF) {
        if (token.type == TF_TOK_RBRACKET) {
            if (p.bracket_depth == 0) {
                fprintf(stderr, "Error in parse: unmatched ']' at position %ld\n",
                        token.str - lexer->program);
                exit(1);
            }
        }
        // parses the token
        tf_word *word = parseToken(&p, &token);
        if (word != NULL) {
            listPush(list, word);
        }
    }

    return list;
}

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

/** Compiles 'program' into a list - STUB VERSION FOR TESTING */
tf_word *compilestub(char *program, size_t length) {
    (void)program;     // Ignora i parametri per ora
    (void)length;

    tf_word *list = createList();

    tf_word *num = createNumber(42);
    listPush(list, num);

    // Push booleano true
    tf_word *boolean = createBoolean(1);
    listPush(list, boolean);

    // Push stringa "hello"
    tf_word *string = createString("hello", 5);
    listPush(list, string);

    tf_word *list2 = createList();
    listPush(list, list2);

    tf_word *num2 = createNumber(12);
    listPush(list2, num2);

    tf_word *string2 = createString("giovanni", 5);
    listPush(list2, string2);

    // Push funzione "print"
    tf_word *print = createFunction("print", 5);
    listPush(list, print);

    return list;
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
