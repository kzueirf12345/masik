#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../structs.h"
#include "elf_map_utils.h"

#define HASH_KEY_ 31
size_t func_hash_func_(const void* const string)
{
    lassert(!is_invalid_ptr(string), "");

    size_t hash_result = 0;
 
    for (const char* it = (const char*)string; *it; ++it)
    {
        hash_result = (size_t)(((HASH_KEY_ * hash_result) % INT64_MAX + (size_t)*it) % INT64_MAX);
    }
 
    return hash_result;
}
#undef HASH_KEY_

int map_key_to_str_ (const void* const elem, const size_t   elem_size,
                            char* const *     str,  const size_t mx_str_size)
{
    if (is_invalid_ptr(str))  return -1;
    if (is_invalid_ptr(*str)) return -1;
    (void)elem_size;

    //FIXME
    // if (elem && *(const char*)elem)
    // {
    //     if (snprintf(*str, mx_str_size, "'%zu, %zu'", 
    //                                     ((const label_t*)elem)->num, 
    //                                     ((const label_t*)elem)->count_args) 
    //         <= 0)
    //     {
    //         perror("Can't snprintf key to str");
    //         return -1;
    //     }
    // }
    // else
    // {
    //     if (snprintf(*str, mx_str_size, "(nul)") < 0)
    //     {
    //         perror("Can't snprintf key (nul) to str");
    //         return -1;
    //     }
    // }

    return 0;
}

int map_val_to_str_ (const void* const elem, const size_t   elem_size,
                            char* const *     str,  const size_t mx_str_size)
{
    if (is_invalid_ptr(str))  return -1;
    if (is_invalid_ptr(*str)) return -1;
    (void)elem_size;

    if (elem)
    {
        if (snprintf(*str, mx_str_size, "'%zu'", *(const size_t*)elem) <= 0)
        {
            perror("Can't snprintf val to str");
            return -1;
        }
    
    }
    else
    {
        if (snprintf(*str, mx_str_size, "(nul)") < 0)
        {
            perror("Can't snprintf key (nul) to str");
            return -1;
        }
    }

    return 0;
}

