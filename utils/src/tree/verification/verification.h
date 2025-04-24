#ifndef MASIK_UTILS_SRC_TREE_VERIFICATION_VERIFIVATION_H
#define MASIK_UTILS_SRC_TREE_VERIFICATION_VERIFIVATION_H

#include <assert.h>

#include "utils/src/tree/structs.h"
#include "utils/src/tree/verification/dumb.h"

enum TreeError
{
    TREE_ERROR_SUCCESS                = 0,
    TREE_ERROR_STANDARD_ERRNO         = 1,
    TREE_ERROR_SYNTAX_ERROR           = 2,
    TREE_ERROR_STACK                  = 3,
    TREE_ERROR_SYNTAXER_IS_NULL       = 4,
    TREE_ERROR_SYNTAXER_IS_INVALID    = 5,
    TREE_ERROR_ELEM_IS_INVALID        = 6,
    TREE_ERROR_SIZE_GREATER           = 7,
    TREE_ERROR_SIZE_LESSER            = 8,
    TREE_ERROR_INVALID_OP_TYPE        = 9,
    TREE_ERROR_INVALID_DATA_NUM       = 10,

    TREE_ERROR_UNKNOWN                = 20,
};
static_assert(TREE_ERROR_SUCCESS == 0);

const char* tree_strerror(const enum TreeError error);

#define TREE_ERROR_HANDLE(call_func, ...)                                                         \
    do {                                                                                            \
        enum TreeError error_handler = call_func;                                                 \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            tree_strerror(error_handler));                                        \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)


enum TreeError tree_verify(const tree_t* const tree);

#ifndef NDEBUG

#define TREE_VERIFY_ASSERT(tree)                                                                   \
        do {                                                                                        \
            const enum TreeError error = tree_verify(tree);                       \
            if (error)                                                                              \
            {                                                                                       \
                tree_dumb(tree);                                                            \
                tree_dumb_dtor();                                                               \
                lassert(false, "Tree error: %s", tree_strerror(error));                           \
            }                                                                                       \
        } while(0)

#else /*NDEBUG*/

#define TREE_VERIFY_ASSERT(tree) do {} while(0)

#endif /*NDEBUG*/

#endif /* MASIK_UTILS_SRC_TREE_VERIFICATION_VERIFIVATION_H */