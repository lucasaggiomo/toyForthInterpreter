#include "parser.h"

// debug, va rimosso
#include <stdlib.h>
#include <stdio.h>

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

tf_word *parseSymbol(tf_token *token) {
    return createFunction(token->str, token->len);
}

tf_word *parseString(tf_token *token) {
    return createString(token->str, token->len);
}

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