#include "word.h"

#include "utils.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>     // debug, va rimosso

const char *getTypeName(tf_type type) {
    switch (type) {
        default:
        case TF_UNKNOWN: return "UNKNOWN";
        case TF_NUMBER: return "NUMBER";
        case TF_BOOLEAN: return "BOOLEAN";
        case TF_STRING: return "STRING";
        case TF_LIST: return "LIST";
        case TF_FUNCTION: return "FUNCTION";
    }
}

/* =============================== CREATIONAL PROCEDURES =============================== */

/** Creates a new tf_word of type `type`. Used by other creational procedures, do not use directly. */
tf_word *createWord(tf_type type) {
    // printf("[DEBUG] Creating word of type %s\n", getTypeName(type));
    tf_word *word = xmalloc(sizeof(*word));
    word->type = type;
    word->refcount = 1;
    return word;
}

/** Frees the allocated tf_word `word`. If word->type == TF_LIST, every word contained in the list is released first.*/
void freeWord(tf_word *word) {
    switch (word->type) {
        case TF_STRING:
        case TF_FUNCTION:
            free(word->str.ptr);
            break;
        case TF_LIST:
            // release every word contained in the list
            for (size_t i = 0; i < word->list.len; i++) {
                releaseWord(word->list.words[i]);
            }
            free(word->list.words);
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

void retainWord(tf_word *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried retaining word with refcount = 0");
        exit(1);
    }
    (word->refcount)++;

    // printf("\t[DEBUG] Retained word");
    // printWord(word, 1);
}

void releaseWord(tf_word *word) {
    if (word->refcount == 0) {
        fprintf(stderr, "Error: tried releasing word with refcount = 0");
        exit(1);
    }
    (word->refcount)--;

    // printf("\t[DEBUG] Released word");
    // printWord(word, 1);

    if (word->refcount == 0) freeWord(word);
}

tf_word *createNumber(int num) {
    tf_word *word = createWord(TF_NUMBER);
    word->num = num;
    return word;
}

tf_word *createBoolean(int value) {
    tf_word *word = createNumber(value);
    word->type = TF_BOOLEAN;
    return word;
}

tf_word *createString(char *string, size_t length) {
    tf_word *word = createWord(TF_STRING);
    word->str.ptr = xmalloc((length + 1) * sizeof(*(word->str.ptr)));
    memcpy(word->str.ptr, string, length);
    word->str.ptr[length] = '\0';
    word->str.len = length;
    return word;
}

tf_word *createFunction(char *funName, size_t length) {
    tf_word *word = createString(funName, length);
    word->type = TF_FUNCTION;
    return word;
}

tf_word *createList() {
    tf_word *word = createWord(TF_LIST);
    word->list.len = 0;
    word->list.capacity = 0;
    word->list.words = NULL;
    return word;
}

/* ================================== LIST OPERATIONS ================================== */

void listPush(tf_word *l, tf_word *word) {
    // checks if there's enough space, otherwise it doubles the capacity
    if (l->list.len == l->list.capacity) {
        size_t newCapacity = (l->list.capacity == 0) ? 8 : l->list.capacity * 2;     // TODO: check overflow
        l->list.words = xrealloc(l->list.words, newCapacity * sizeof(*(l->list.words)));
        l->list.capacity = newCapacity;
    }

    l->list.words[l->list.len] = word;
    (l->list.len)++;
}

tf_word *listPop(tf_word *l) {
    if (l->list.len == 0)
        return NULL;

    (l->list.len)--;
    return l->list.words[l->list.len];
}

tf_word *listPeek(tf_word *l) {
    if (l->list.len == 0)
        return NULL;

    tf_word *word = l->list.words[l->list.len - 1];
    retainWord(word);

    return word;
}