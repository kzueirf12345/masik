#ifndef MASIK_SYNTAXER_VERIFICATION_DUMB_H
#define MASIK_SYNTAXER_VERIFICATION_DUMB_H

#include <assert.h>

#include "syntaxer/structs.h"

enum SyntaxerDumbError
{
    SYNTAXER_DUMB_ERROR_SUCCESS = 0,
    SYNTAXER_DUMB_ERROR_FAILURE = 1
};
static_assert(SYNTAXER_DUMB_ERROR_SUCCESS == 0);

const char* syntaxer_dumb_strerror(const enum SyntaxerDumbError error);

#define SYNTAXER_DUMB_ERROR_HANDLE(call_func, ...)                                                  \
    do {                                                                                            \
        enum SyntaxerDumbError error_handler = call_func;                                           \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            syntaxer_dumb_strerror(error_handler));                                 \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

enum SyntaxerDumbError syntaxer_dumb_ctor(void);
enum SyntaxerDumbError syntaxer_dumb_dtor(void);

enum SyntaxerDumbError syntaxer_dumb_set_out_file(char* const filename);

void syntaxer_dumb (const syntaxer_t* const syntaxer);

#endif /* MASIK_SYNTAXER_VERIFICATION_DUMB_H */