#include "compiler.h"

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