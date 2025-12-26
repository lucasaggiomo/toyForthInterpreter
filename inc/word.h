#ifndef _WORD_H_
#define _WORD_H_

#include <stddef.h>

enum tf_type {
    TF_UNKNOWN = 0,
    TF_NUMBER = 1 << 0,
    TF_BOOLEAN = 1 << 1,
    TF_STRING = 1 << 2,
    TF_LIST = 1 << 3,
    TF_FUNCTION = 1 << 4
};
typedef enum tf_type tf_type;

// restituisce 0 se obj non è un tipo valido tra quelli presenti nella maschera (oppure se mask == 0)
#define TF_VALID_TYPE(obj, mask) (((obj)->type & (mask)))

#define MAX_FUN_LEN 32
#define MAX_STR_LEN 256

const char *getTypeName(tf_type type);

/**
 * The code to interpret is modeled as a list of `words`.
 * Whenever an operand is encountered on a command, it is pushed on the stack.
 * A WORD can be:
 *  - a TF_NUMBER (only integer for now). A character is interpreted as an integer whose value corresponds to the ASCII value of the character.
 *  - a TF_STRING (series of characters of a fixed length) [terminated by null character (?)]
 *  - a TF_LIST, defined as an ordered sequence of words. Note that a COMMAND is modeled as a list.
 *  - a TF_FUNCTION (`+` in the last example). It is just a symbol that represents something to execute.
 *
 * A word is composed by:
 *  - an HEADER, that contains all the relevant metadata regarding the word
 *  - a PAYLOAD, containing the word's data
 */
typedef struct tf_word {
    // HEADER
    int refcount;
    tf_type type;

    // PAYLOAD
    union {
        // TF_NUMBER | TF_BOOLEAN
        int num;

        // TF_STRING | TF_FUNCTION
        struct {
            size_t len;
            char *ptr;
        } str;

        // TF_LIST: modeled as an array of type word
        struct {
            size_t len;
            size_t capacity;
            struct tf_word **words;     // array dinamico di puntatori a tf_word
        } list;
    };
} tf_word;

/* =============================== CREATIONAL PROCEDURES =============================== */

/**
 * Retains the ownership of the tf_word `word`, incrementing the refcount of such word.
 * It requires to call `releaseWord` after using the word.
 */
void retainWord(tf_word *word);

/**
 * Releases the ownership of the tf_word `word`, decrementing the refcount of such word.
 * It is unsafe to use `word` afterwards, because this function could free the word.
 */
void releaseWord(tf_word *word);

tf_word *createNumber(int num);
tf_word *createBoolean(int value);

/**
 * Creates a new tf_word of type TF_STRING, copying exactly `length` bytes from `string` in the new tf_word created.
 * Therefore the input string doesn't need to be NULL terminated, as the NULL terminator doesn't stop the copy.
 * The created string is NULL terminated.
 */
tf_word *createString(char *string, size_t length);
tf_word *createFunction(char *funName, size_t length);
tf_word *createList();

/* ================================== LIST OPERATIONS ================================== */

/**
 * Pushes `word` on the end of the list `l`, reallocating the necessary space if needed.
 * The ownership of the word is transferred to the list, assuming the caller wants to release it.
 * Otherwise the caller needs to manually retain the word again.
 */
void listPush(tf_word *l, tf_word *word);

/**
 * Pops the word at the end of the list `l`, or NULL if list is empty.
 * The ownership of the word is transferred to the caller.
 * Therefore, the caller has to release the word at the end of usage.
 */
tf_word *listPop(tf_word *l);

/**
 * Peeks the word at the end of the list `l`, or NULL if list is empty.
 * The word is automatically retained for the caller, because the word remains in the list.
 * Therefore, the caller has to release the word at the end of usage.
 */
tf_word *listPeek(tf_word *l);

#endif     // _WORD_H_