#ifndef _LEXER_H_
#define _LEXER_H_

#include <stddef.h>

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
void skipSpaces(tf_lexer *lexer);

int isValidSymbol(int c);

/** Finds the next token of the source code from lexer->next, and updates this pointer. */
tf_token findNextToken(tf_lexer *lexer);

#endif     // _LEXER_H_