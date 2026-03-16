#include "utils.h"

#include <stdio.h>     // debug, va rimosso
#include <stdlib.h>

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