#ifndef MASIK_FRONTEND_SRC_LEXER_STRUCTS_H
#define MASIK_FRONTEND_SRC_LEXER_STRUCTS_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "stack_on_array/libstack.h"
#include "utils/utils.h"

typedef struct Lexer
{
    stack_key_t stack;
} lexer_t;

#endif /*MASIK_FRONTEND_SRC_LEXER_STRUCTS_H*/