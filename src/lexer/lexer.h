#ifndef MASIK_SRC_LEXER_LEXER_H
#define MASIK_SRC_LEXER_LEXER_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "operations/operations.h"
#include "stack_on_array/libstack.h"

enum LexerError
{
    LEXER_ERROR_SUCCESS              = 0,
    LEXER_ERROR_STANDARD_ERRNO       = 1,
    LEXER_ERROR_STACK                = 2,
    LEXER_ERROR_INVALID_LEXEM        = 3,
};
static_assert(LEXER_ERROR_SUCCESS == 0);

const char* lexer_strerror(const enum LexerError error);

#define LEXER_ERROR_HANDLE(call_func, ...)                                                          \
    do {                                                                                            \
        enum LexerError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            lexer_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)


typedef int64_t num_t;

enum LexemType
{
    LEXEM_TYPE_END = 0,
    LEXEM_TYPE_OP  = 1,
    LEXEM_TYPE_NUM = 2,
};
static_assert(LEXEM_TYPE_END == 0);

const char* lexem_type_to_str(const enum LexemType type);

typedef union LexemData
{
    num_t num;
    enum OpType op;
} lexem_data_u;

typedef struct Lexem
{
    enum LexemType type;
    lexem_data_u data;
} lexem_t;


typedef struct Lexer
{
    stack_key_t stack;
} lexer_t;

enum LexerError lexer_ctor(lexer_t* const lexer);
void            lexer_dtor(lexer_t* const lexer);
enum LexerError lexer_push(lexer_t* const lexer, const lexem_t lexem);
lexem_t*        lexer_get (lexer_t        lexer, const size_t ind);

enum LexerError lexing(lexer_t* const lexer, const char* const filename);

#endif /* MASIK_SRC_LEXER_LEXER_H */