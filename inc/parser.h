#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"
#include "word.h"

struct tf_parser {
    tf_lexer *lexer;
    int bracket_depth;
};
typedef struct tf_parser tf_parser;

/**
 * Parses a number word from 'token'. Assumes that token->type == TK_TOK_NUMBER.
 * Returns the tf_word* containing the parsed number.
 */
tf_word *parseNumber(tf_token *token);

/**
 * Parses a symbol word from 'token'. Assumes that token->type == TF_TOK_IDENTIFIER.
 * Returns the tf_word* containing the parsed symbol name.
 */
tf_word *parseSymbol(tf_token *token);

/**
 * Parses a string word from 'token'. Assumes that token->type == TF_TOK_STRING.
 * Returns the tf_word* containing the parsed string.
 */
tf_word *parseString(tf_token *token);

/** Given a token and a lexer, creates an associated tf_word object. Returns NULL if token type is TF_TOK_EOF or TF_TOK_RBRACKET. */
tf_word *parseToken(tf_parser *p, tf_token *token);

/** Parses the program from a given lexer and creates a list containing the words that model the program. */
tf_word *parse(tf_lexer *lexer);

#endif		// _PARSER_H_