#ifndef MASIK_MIDLEND_SRC_MODIFICATION_MODIFICATION_H
#define MASIK_MIDLEND_SRC_MODIFICATION_MODIFICATION_H

#include "../flags/flags.h"

enum ModificationError
{
    MODIFICATION_ERROR_SUCCESS               = 0,
    MODIFICATION_ERROR_STANDARD_ERRNO        = 1,
};
static_assert(MODIFICATION_ERROR_SUCCESS == 0);

const char* modification_strerror(const enum ModificationError error);

#define MODIFICATION_ERROR_HANDLE(call_func, ...)                                                   \
    do {                                                                                            \
        enum ModificationError error_handler = call_func;                                           \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            modification_strerror(error_handler));                                  \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

enum TreeError tree_modify(tree_t* const tree, const enum Mode mode);

#endif /* MASIK_MIDLEND_SRC_MODIFICATION_MODIFICATION_H */