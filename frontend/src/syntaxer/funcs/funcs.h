#ifndef MASIK_FRONTED_SRC_SYNTAXER_FUNCS_FUNCS_H
#define MASIK_FRONTED_SRC_SYNTAXER_FUNCS_FUNCS_H

#include "utils/src/tree/structs.h"
#include "utils/src/tree/verification/verification.h"
#include "lexer/structs.h"

enum TreeError syntaxer_ctor(tree_t* const syntaxer, const lexer_t lexer);

#endif /* MASIK_FRONTED_SRC_SYNTAXER_FUNCS_FUNCS_H */