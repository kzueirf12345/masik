#ifndef MASIK_IR_BACKEND_SRC_TRANSLATION_STUCTS_H
#define MASIK_IR_BACKEND_SRC_TRANSLATION_STUCTS_H

#include <stdlib.h>
#include <stdio.h>

#include "hash_table/libhash_table.h"
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
    size_t temp_var_num;
    long long int var_num_base;
    size_t arg_var_num;

    smash_map_t func_arg_num;
} translator_t;

#endif /*MASIK_IR_BACKEND_SRC_TRANSLATION_STUCTS_H*/