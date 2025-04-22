#ifndef MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H
#define MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H

#include <stdio.h>

#include "utils/src/tree/structs.h"
#include "translation/verification/verification.h"

enum TranslationError translate_splu(const tree_t* const tree, FILE* out);

enum TranslationError translate_nasm(const tree_t* const tree, FILE* out);


#endif /* MASIK_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H */