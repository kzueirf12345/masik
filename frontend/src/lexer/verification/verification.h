#ifndef MASIK_FRONTEND_SRC_LEXER_VERIFICATION_VERIFICATION_H
#define MASIK_FRONTEND_SRC_LEXER_VERIFICATION_VERIFICATION_H

#include <assert.h>

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

#endif /*MASIK_FRONTEND_SRC_LEXER_VERIFICATION_VERIFICATION_H*/