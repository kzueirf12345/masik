#ifndef MASIK_IR_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H
#define MASIK_IR_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H

#include <stdio.h>

#include "utils/src/tree/structs.h"
#include "translation/verification/verification.h"

enum IrTranslationError translate(const tree_t* const tree, FILE* out);


#endif /* MASIK_IR_BACKEND_SRC_TRANSLATION_FUNCS_FUNCS_H */