#include "verification.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* ir_fist_strerror(const enum IrFistError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(IR_FIST_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(IR_FIST_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(IR_FIST_ERROR_PARSE_BLOCK);
        CASE_ENUM_TO_STRING_(IR_FIST_ERROR_FIST);
        default:
            return "UNKNOWN_IR_FIST_ERROR";
    }
    return "UNKNOWN_IR_FIST_ERROR";
}
#undef CASE_ENUM_TO_STRING_