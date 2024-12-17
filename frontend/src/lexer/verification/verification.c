#include "lexer/verification/verification.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* lexer_strerror(const enum LexerError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(LEXER_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_STACK);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_INVALID_LEXEM);

        default:
            return "UNKNOWN_LEXER_ERROR";
    }
    return "UNKNOWN_LEXER_ERROR";
}
#undef CASE_ENUM_TO_STRING_