#include "parser.h"

// debug, va rimosso
#include <stdio.h>
#include <stdlib.h>

TfWord *parseNumber(TfToken *token) {
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

TfWord *parseSymbol(TfToken *token) {
    return createFunction(token->str, token->len);
}

TfWord *parseString(TfToken *token) {
    return createString(token->str, token->len);
}

TfWord *parseToken(TfParser *p, TfToken *token) {
    TfWord *word = NULL;

    switch (token->kind) {
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
            TfToken next;
            while ((next = findNextToken(p->lexer)).kind != TF_TOK_RBRACKET) {
                if (next.kind == TF_TOK_EOF) {
                    fprintf(stderr, "Error in parseToken: unclosed bracket (reached EOF)\n");
                    exit(1);
                }
                TfWord *element = parseToken(p, &next);
                if (element != NULL) {
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

TfWord *parse(TfLexer *lexer) {
    TfWord *list = createList();

    TfToken token;
    TfParser p = { .lexer = lexer, .bracket_depth = 0 };
    while ((token = findNextToken(lexer)).kind != TF_TOK_EOF) {
        if (token.kind == TF_TOK_RBRACKET) {
            if (p.bracket_depth == 0) {
                fprintf(stderr, "Error in parse: unmatched ']' at position %ld\n",
                        token.str - lexer->program);
                exit(1);
            }
        }
        // parses the token
        TfWord *word = parseToken(&p, &token);
        if (word != NULL) {
            listPush(list, word);
        }
    }

    return list;
}