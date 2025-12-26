#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "parser.h"

/**
 * Compiles 'program' into a list.
 */
tf_word *compile(char *program, size_t length);

#endif		// _COMPILER_H_