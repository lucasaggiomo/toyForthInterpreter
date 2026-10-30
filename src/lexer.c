#include "lexer.h"

#include <ctype.h>
#include <string.h>

// debug, va rimosso
#include <stdio.h>
#include <stdlib.h>

void skipSpaces(TfLexer *lexer) {
    while (isspace((unsigned char)lexer->next[0])) {
        (lexer->next)++;
    }
}

int isValidSymbol(int c) {
    static char *validSymbols = "+-*/%><=";
    return c != '\0' && (isalpha((unsigned char)c) || strchr(validSymbols, c) != NULL);
}

TfToken findNextToken(TfLexer *lexer) {
    if (lexer->next == NULL)
        return (TfToken) { TF_TOK_EOF, lexer->next, 0 };

    // skips starting spaces
    skipSpaces(lexer);

#ifdef DEBUG
    printf("[DEBUG]: lexer->next = '%c' (offset %ld)\n",
           lexer->next[0], lexer->next - lexer->program);
#endif

    if (lexer->next[0] == '\0')
        return (TfToken) { TF_TOK_EOF, lexer->next, 0 };

    /* Detects kind of token encountered:
     *  - letter (between 'A' and 'Z' or between 'a' and 'z') => TF_TOK_IDENTIFIER
     *  - digit (between '0' and '9') or '-' => TF_TOK_NUMBER
     *  - character '"' => TF_TOK_STRING
     *  - character '[' => TF_TOK_LBRACKET
     *  - character ']' => TF_TOK_RBRACKET
     */

    char *start = lexer->next;
    size_t len = 0;
    TfTokenKind kind = TF_TOK_ERROR;
    if (isdigit((unsigned char)lexer->next[0]) || (lexer->next[0] == '-' && isdigit((unsigned char)lexer->next[1]))) {

        kind = TF_TOK_NUMBER;
        if (lexer->next[0] == '-')
            (lexer->next)++;
        while (isdigit((unsigned char)lexer->next[0])) {
            (lexer->next)++;
        }
        len = lexer->next - start;

    } else if (isValidSymbol(lexer->next[0])) {

        kind = TF_TOK_IDENTIFIER;
        while (isValidSymbol(lexer->next[0])) {
            (lexer->next)++;
        }
        len = lexer->next - start;

    } else if (lexer->next[0] == '\"') {

        kind = TF_TOK_STRING;
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

        kind = TF_TOK_LBRACKET;
        (lexer->next)++;
        len = lexer->next - start;

    } else if (lexer->next[0] == ']') {

        kind = TF_TOK_RBRACKET;
        (lexer->next)++;
        len = lexer->next - start;

    } else if (lexer->next[0] == '#') {

        kind = TF_TOK_COMMENT;
        start = ++(lexer->next);     // increments lexer->next to skip starting '#'. Then updates start

        // searches for newline character or eof
        while (lexer->next[0] != '\n' && lexer->next[0] != '\0') {
            (lexer->next)++;
        }

        // skips '\n'
        if (lexer->next[0] == '\n')
            (lexer->next)++;

        len = lexer->next - start;

    } else {
        fprintf(stderr, "Error in findNextToken: unrecognized starting character at position %ld: \'%c\'.\nNext 32 characters between quotes:\n\'%32s\'\n",
                lexer->next - lexer->program, lexer->next[0], lexer->next);
        exit(1);
    }

    return (TfToken) { kind, start, len };
}
