#include "word.h"

#include "utils.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>     // debug, va rimosso

const char *getTypeName(TfType type) {
    switch (type) {
        default:
        case TF_UNKNOWN:
            return "UNKNOWN";
        case TF_NUMBER:
            return "NUMBER";
        case TF_BOOLEAN:
            return "BOOLEAN";
        case TF_STRING:
            return "STRING";
        case TF_LIST:
            return "LIST";
        case TF_FUNCTION:
            return "FUNCTION";
    }
}

/* =============================== CREATIONAL PROCEDURES =============================== */

/**
 * Creates a new TfWord of type `type`. Used by other creational procedures, do not use directly.
 * The TfWord::as union is zeroed.
 */
TfWord *createWord(TfType type) {
    // printf("[DEBUG] Creating word of type %s\n", getTypeName(type));
    TfWord *word = xmalloc(sizeof(*word));
    word->type = type;
    word->refcount = 1;
    memset(&word->as, 0, sizeof(word->as));
    return word;
}

/** Frees the allocated TfWord `word`. If word->type == TF_LIST, every word contained in the list is released first.*/
void freeWord(TfWord *word) {
    switch (word->type) {
        case TF_STRING:
        case TF_FUNCTION:
            sbFree(&word->as.str);
            break;

        case TF_LIST:
            // release every word contained in the list
            for (size_t i = 0; i < word->as.list.len; i++) {
                releaseWord(word->as.list.words[i]);
            }
            free(word->as.list.words);
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

void retainWord(TfWord *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried retaining word with refcount = 0");
        exit(1);
    }
    (word->refcount)++;

    // printf("\t[DEBUG] Retained word");
    // printWord(word, 1);
}

void releaseWord(TfWord *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried releasing word with refcount = 0");
        exit(1);
    }
    (word->refcount)--;

    // printf("\t[DEBUG] Released word");
    // printWord(word, 1);

    if (word->refcount == 0) freeWord(word);
}

TfWord *createNumber(int num) {
    TfWord *word = createWord(TF_NUMBER);
    word->as.num = num;
    return word;
}

TfWord *createBoolean(int value) {
    TfWord *word = createNumber(value);
    word->type = TF_BOOLEAN;
    return word;
}

TfWord *createString(char *string, size_t length) {
    TfWord *word = createWord(TF_STRING);
    word->as.str = sbCreate(string, length);
    return word;
}

TfWord *createFunction(char *funName, size_t length) {
    TfWord *word = createString(funName, length);
    word->type = TF_FUNCTION;
    return word;
}

TfWord *createList() {
    TfWord *word = createWord(TF_LIST);
    word->as.list.len = 0;
    word->as.list.capacity = 0;
    word->as.list.words = NULL;
    return word;
}

/* ================================== LIST OPERATIONS ================================== */

void listPush(TfWord *l, TfWord *word) {
    // checks if there's enough space, otherwise it doubles the capacity
    if (l->as.list.len == l->as.list.capacity) {
        size_t newCapacity = (l->as.list.capacity == 0) ? 8 : l->as.list.capacity * 2;     // TODO: check overflow
        l->as.list.words = xrealloc(l->as.list.words, newCapacity * sizeof(*(l->as.list.words)));
        l->as.list.capacity = newCapacity;
    }

    l->as.list.words[l->as.list.len] = word;
    (l->as.list.len)++;
}

TfWord *listPop(TfWord *l) {
    if (l->as.list.len == 0)
        return NULL;

    (l->as.list.len)--;
    return l->as.list.words[l->as.list.len];
}

TfWord *listPeek(TfWord *l) {
    if (l->as.list.len == 0)
        return NULL;

    TfWord *word = l->as.list.words[l->as.list.len - 1];
    retainWord(word);

    return word;
}