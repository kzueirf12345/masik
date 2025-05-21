#ifndef MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_MAP_UTILS_H
#define MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_MAP_UTILS_H

#include <stdlib.h>

#include "hash_table/libhash_table.h"
#include "translation/verification/verification.h"

size_t func_hash_func(const void* const string);
int map_key_to_str (const void* const elem, const size_t   elem_size,
                     char* const *     str,  const size_t mx_str_size);
int map_val_to_str (const void* const elem, const size_t   elem_size,
                     char* const *     str,  const size_t mx_str_size);

#define SMASH_MAP_ERROR_HANDLE_(call_func, ...)                                                     \
    do {                                                                                            \
        const enum SmashMapError error_handler = call_func;                                         \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". SmashMap error: %s\n",                            \
                            smash_map_strerror(error_handler));                                     \
            __VA_ARGS__                                                                             \
            return TRANSLATION_ERROR_SMASH_MAP;                                                     \
        }                                                                                           \
    } while(0)

#endif /*MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_MAP_UTILS_H*/