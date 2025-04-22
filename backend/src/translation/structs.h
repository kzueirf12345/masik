#ifndef MASIK_BACKEND_SRC_TRANSLATION_TRANSLATION_H
#define MASIK_BACKEND_SRC_TRANSLATION_TRANSLATION_H

#include <stdlib.h>

#include "stack_on_array/libstack.h"

typedef struct Func
{
    size_t num;
    size_t count_args;
} func_t;

typedef struct Translator
{
    stack_key_t vars;
    stack_key_t funcs;
    size_t label_num;
    size_t count_var_decl;
} translator_t;

#endif /*MASIK_BACKEND_SRC_TRANSLATION_TRANSLATION_H*/