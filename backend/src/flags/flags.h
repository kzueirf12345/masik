#ifndef MASIK_BACKEND_SRC_FLAGS_FLAGS_H
#define MASIK_BACKEND_SRC_FLAGS_FLAGS_H

#include "utils/utils.h"

enum FlagsError
{
    FLAGS_ERROR_SUCCESS     = 0,
    FLAGS_ERROR_FAILURE     = 1,
};
static_assert(FLAGS_ERROR_SUCCESS  == 0);

const char* flags_strerror(const enum FlagsError error);

#define FLAGS_ERROR_HANDLE(call_func, ...)                                                          \
    do {                                                                                            \
        enum FlagsError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            flags_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return error_handler;                                                                   \
        }                                                                                           \
    } while(0)

typedef struct FlagsObjs
{
    char log_folder [FILENAME_MAX];

    char in_filename[FILENAME_MAX];
    char splu_filename[FILENAME_MAX];
    char nasm_filename[FILENAME_MAX];

    FILE* splu_out;
    FILE* nasm_out;
} flags_objs_t;

enum FlagsError flags_objs_ctor (flags_objs_t* const flags_objs);
enum FlagsError flags_objs_dtor (flags_objs_t* const flags_objs);
enum FlagsError flags_processing(flags_objs_t* const flags_objs, 
                                 const int argc, char* const argv[]);



#endif /*MASIK_BACKEND_SRC_FLAGS_FLAGS_H*/