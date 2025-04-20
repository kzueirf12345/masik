#include <stdio.h>

#include "tree/verification/verification.h"
#include "utils/utils.h"
#include "logger/liblogger.h"
#include "tree/structs.h"


#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* tree_strerror(const enum TreeError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(TREE_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(TREE_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(TREE_ERROR_SYNTAX_ERROR);
        CASE_ENUM_TO_STRING_(TREE_ERROR_STACK);
        CASE_ENUM_TO_STRING_(TREE_ERROR_SYNTAXER_IS_NULL);
        CASE_ENUM_TO_STRING_(TREE_ERROR_SYNTAXER_IS_INVALID);
        CASE_ENUM_TO_STRING_(TREE_ERROR_ELEM_IS_INVALID);
        CASE_ENUM_TO_STRING_(TREE_ERROR_SIZE_GREATER);
        CASE_ENUM_TO_STRING_(TREE_ERROR_SIZE_LESSER);
        CASE_ENUM_TO_STRING_(TREE_ERROR_INVALID_OP_TYPE);
        CASE_ENUM_TO_STRING_(TREE_ERROR_INVALID_DATA_NUM);
        CASE_ENUM_TO_STRING_(TREE_ERROR_UNKNOWN);

        default:
            return "UNKNOWN_TREE_ERROR";
    }
    return "UNKNOWN_TREE_ERROR";
}
#undef CASE_ENUM_TO_STRING_

enum TreeError tree_verify_recursive_(const tree_elem_t* const elem, size_t* const size);

enum TreeError tree_verify_NOT_USE(const tree_t* const syntaxer)
{
    switch (is_invalid_ptr(syntaxer))
    {
        case PTR_STATES_VALID:       break;
        case PTR_STATES_NULL:        return TREE_ERROR_SYNTAXER_IS_NULL;
        case PTR_STATES_INVALID:     return TREE_ERROR_SYNTAXER_IS_INVALID;
        case PTR_STATES_ERROR:       return TREE_ERROR_STANDARD_ERRNO;
        
        default:
            fprintf(stderr, "Unknown PtrState enum syntaxer, it's soooo bad\n");
            return TREE_ERROR_UNKNOWN;
    }

    size_t size = 0;

    TREE_ERROR_HANDLE(tree_verify_recursive_(syntaxer->Groot, &size));

    if (syntaxer->size > size) return TREE_ERROR_SIZE_GREATER;
    if (syntaxer->size < size) return TREE_ERROR_SIZE_LESSER;

    return TREE_ERROR_SUCCESS;
}

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case num_: break;

enum TreeError tree_verify_recursive_(const tree_elem_t* const elem, size_t* const size)
{
    if (!elem) return TREE_ERROR_SUCCESS;

    switch (is_invalid_ptr(elem))
    {
        case PTR_STATES_VALID:       break;
        case PTR_STATES_NULL:        return TREE_ERROR_SUCCESS;
        case PTR_STATES_INVALID:     return TREE_ERROR_ELEM_IS_INVALID;
        case PTR_STATES_ERROR:       return TREE_ERROR_STANDARD_ERRNO;
        
        default:
            fprintf(stderr, "Unknown PtrState enum syntax elem, it's soooo bad\n");
            return TREE_ERROR_UNKNOWN;
    }

    if (elem->lexem.type == LEXEM_TYPE_OP)
    {
        switch(elem->lexem.data.op)
        {
            #include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                return TREE_ERROR_INVALID_OP_TYPE;
        }
    }

    ++*size;

    TREE_ERROR_HANDLE(tree_verify_recursive_(elem->lt, size));
    TREE_ERROR_HANDLE(tree_verify_recursive_(elem->rt, size));

    return TREE_ERROR_SUCCESS;
}



