#include "compiler.h"

TfWord *compile(char *program, size_t length) {
    (void)length;

    // creates a lexer and gets the next token. Then the token is passed to the parser, that creates the data structure of words modeling the program

    TfLexer lexer = {
        .program = program,
        .next = program
    };

    TfWord *list = parse(&lexer);

    return list;
}