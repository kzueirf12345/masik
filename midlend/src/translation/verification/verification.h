#ifndef MASIK_IR_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H
#define MASIK_IR_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H

#include <assert.h>

enum IrTranslationError
{
    IR_TRANSLATION_ERROR_SUCCESS               = 0,
    IR_TRANSLATION_ERROR_STANDARD_ERRNO        = 1,
    IR_TRANSLATION_ERROR_STACK                 = 2, 
    IR_TRANSLATION_ERROR_UNDECL_VAR            = 3,
    IR_TRANSLATION_ERROR_INVALID_OP_TYPE       = 4,
    IR_TRANSLATION_ERROR_INVALID_LEXEM_TYPE    = 5,
    IR_TRANSLATION_ERROR_REDECL_VAR            = 6,
    IR_TRANSLATION_ERROR_SMASH_MAP             = 7,
};
static_assert(IR_TRANSLATION_ERROR_SUCCESS == 0, "");

const char* ir_translation_strerror(const enum IrTranslationError error);

#define IR_TRANSLATION_ERROR_HANDLE(call_func, ...)                                                 \
    do {                                                                                            \
        enum IrTranslationError error_handler = call_func;                                          \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            ir_translation_strerror(error_handler));                                \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#endif /* MASIK_IR_BACKEND_SRC_TRANSLATION_VERIFICATION_VARIFICATION_H */