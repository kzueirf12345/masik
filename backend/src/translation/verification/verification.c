#include "translation/verification/verification.h"


#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* translation_strerror(const enum TranslationError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_STACK);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_UNDECL_VAR);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_INVALID_OP_TYPE);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_INVALID_LEXEM_TYPE);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_REDECL_VAR);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_SMASH_MAP);
        default:
            return "UNKNOWN_TRANSLATION_ERROR";
    }
    return "UNKNOWN_TRANSLATION_ERROR";
}
#undef CASE_ENUM_TO_STRING_