#ifndef MASIK_BACKEND_SRC_IR_FIST_VERIFICATION_VERIFICATION
#define MASIK_BACKEND_SRC_IR_FIST_VERIFICATION_VERIFICATION

#include <assert.h>

enum IrFistError
{
    IR_FIST_ERROR_SUCCESS               = 0,
    IR_FIST_ERROR_STANDARD_ERRNO        = 1,
    IR_FIST_ERROR_PARSE_BLOCK           = 2,
    IR_FIST_ERROR_FIST                  = 3,
};
static_assert(IR_FIST_ERROR_SUCCESS == 0, "");

const char* ir_fist_strerror(const enum IrFistError error);

#define IR_FIST_ERROR_HANDLE(call_func, ...)                                                        \
    do {                                                                                            \
        enum IrFistError error_handler = call_func;                                                 \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            ir_fist_strerror(error_handler));                                       \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

#endif /* MASIK_BACKEND_SRC_IR_FIST_VERIFICATION_VERIFICATION */