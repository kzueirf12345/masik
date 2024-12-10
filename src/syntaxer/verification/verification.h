#ifndef MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H
#define MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H

#include <assert.h>

#include "syntaxer/structs.h"

enum SyntaxError
{
    SYNTAX_ERROR_SUCCESS        = 0,
    SYNTAX_ERROR_STANDARD_ERRNO = 1,
    SYNTAX_ERROR_SYNTAX_ERROR   = 2,
    SYNTAX_ERROR_STACK          = 3,
};
static_assert(SYNTAX_ERROR_SUCCESS == 0);

const char* syntax_strerror(const enum SyntaxError error);

#define SYNTAX_ERROR_HANDLE(call_func, ...)                                                         \
    do {                                                                                            \
        enum SyntaxError error_handler = call_func;                                                 \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            syntax_strerror(error_handler));                                        \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)


#endif /* MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H */