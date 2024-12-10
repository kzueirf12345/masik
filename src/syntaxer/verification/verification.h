#ifndef MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H
#define MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H

#include <assert.h>

#include "syntaxer/structs.h"
#include "syntaxer/verification/dumb.h"

enum SyntaxError
{
    SYNTAX_ERROR_SUCCESS                = 0,
    SYNTAX_ERROR_STANDARD_ERRNO         = 1,
    SYNTAX_ERROR_SYNTAX_ERROR           = 2,
    SYNTAX_ERROR_STACK                  = 3,
    SYNTAX_ERROR_SYNTAXER_IS_NULL       = 4,
    SYNTAX_ERROR_SYNTAXER_IS_INVALID    = 5,
    SYNTAX_ERROR_ELEM_IS_INVALID        = 6,
    SYNTAX_ERROR_SIZE_GREATER           = 7,
    SYNTAX_ERROR_SIZE_LESSER            = 8,
    SYNTAX_ERROR_INVALID_OP_TYPE        = 9,

    SYNTAX_ERROR_UNKNOWN                = 20,
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

#ifndef NDEBUG

enum SyntaxError syntaxer_verify_NOT_USE(const syntaxer_t* const syntaxer);

#define SYNTAXER_VERIFY(syntaxer)                                                                   \
        do {                                                                                        \
            const enum SyntaxError error = syntaxer_verify_NOT_USE(syntaxer);                       \
            if (error)                                                                              \
            {                                                                                       \
                syntaxer_dumb(syntaxer);                                                            \
                syntaxer_dumb_dtor();                                                               \
                lassert(false, "Tree error: %s", syntax_strerror(error));                           \
            }                                                                                       \
        } while(0)

#else /*NDEBUG*/

#define SYNTAXER_VERIFY(syntaxer) do {} while(0)

#endif /*NDEBUG*/

#endif /* MASIK_SYNTAXER_VERIFICATION_VERIFIVATION_H */