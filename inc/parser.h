#ifndef _TF_PARSER_H_
#define _TF_PARSER_H_

#include "lexer.h"
#include "word.h"

typedef struct TfParser {
    TfLexer *lexer;
    int bracket_depth;
} TfParser;

/**
 * Parses a number word from 'token'. Assumes that token->kind == TK_TOK_NUMBER.
 * Returns the TfWord* containing the parsed number.
 */
TfWord *parseNumber(TfToken *token);

/**
 * Parses a symbol word from 'token'. Assumes that token->kind == TF_TOK_IDENTIFIER.
 * Returns the TfWord* containing the parsed symbol name.
 */
TfWord *parseSymbol(TfToken *token);

/**
 * Parses a string word from 'token'. Assumes that token->kind == TF_TOK_STRING.
 * Returns the TfWord* containing the parsed string.
 */
TfWord *parseString(TfToken *token);

/** Given a token and a lexer, creates an associated TfWord object. Returns NULL if token kind is TF_TOK_EOF or TF_TOK_RBRACKET. */
TfWord *parseToken(TfParser *p, TfToken *token);

/** Parses the program from a given lexer and creates a list containing the words that model the program. */
TfWord *parse(TfLexer *lexer);

#endif     // _TF_PARSER_H_