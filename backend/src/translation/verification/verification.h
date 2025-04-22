#ifndef MASIK_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H
#define MASIK_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H

#include <assert.h>

enum TranslationError
{
    TRANSLATION_ERROR_SUCCESS               = 0,
    TRANSLATION_ERROR_STANDARD_ERRNO        = 1,
    TRANSLATION_ERROR_STACK                 = 2, 
    TRANSLATION_ERROR_UNDECL_VAR            = 3,
    TRANSLATION_ERROR_INVALID_OP_TYPE       = 4,
    TRANSLATION_ERROR_INVALID_LEXEM_TYPE    = 5,
    TRANSLATION_ERROR_REDECL_VAR            = 6,
};
static_assert(TRANSLATION_ERROR_SUCCESS == 0, "");

const char* translation_strerror(const enum TranslationError error);

#define TRANSLATION_ERROR_HANDLE(call_func, ...)                                                    \
    do {                                                                                            \
        enum TranslationError error_handler = call_func;                                            \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            translation_strerror(error_handler));                                   \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#endif /* MASIK_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H */