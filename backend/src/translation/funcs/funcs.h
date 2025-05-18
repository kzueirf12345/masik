#ifndef MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H
#define MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H

#include <stdio.h>

#include "utils/src/tree/structs.h"
#include "../verification/verification.h"
#include "hash_table/libs/list_on_array/libfist.h"

enum TranslationError translate_splu(const fist_t* const fist, FILE* out);

enum TranslationError translate_nasm(const fist_t* const fist, FILE* out);


#endif /* MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H */