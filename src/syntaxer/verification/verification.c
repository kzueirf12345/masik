
#include "syntaxer/verification/verification.h"
#include "utils/utils.h"
#include "logger/liblogger.h"
#include "syntaxer/structs.h"


#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* syntax_strerror(const enum SyntaxError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(SYNTAX_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(SYNTAX_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(SYNTAX_ERROR_SYNTAX_ERROR);
        CASE_ENUM_TO_STRING_(SYNTAX_ERROR_STACK);

        default:
            return "UNKNOWN_SYNTAX_ERROR";
    }
    return "UNKNOWN_SYNTAX_ERROR";
}
#undef CASE_ENUM_TO_STRING_



