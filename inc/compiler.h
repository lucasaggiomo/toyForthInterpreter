#ifndef _TF_COMPILER_H_
#define _TF_COMPILER_H_

#include "word.h"

/** Compiles 'program' into a list. */
TfWord *compile(char *program, size_t length);

#endif     // _TF_COMPILER_H_