#ifndef MASIK_SRC_SYNTAXER_FUNCS_FUNCS_H
#define MASIK_SRC_SYNTAXER_FUNCS_FUNCS_H

#include "syntaxer/structs.h"
#include "syntaxer/verification/verification.h"

syntax_elem_t* syntax_elem_ctor(lexem_t lexem, syntax_elem_t* lt, syntax_elem_t* rt);
void           syntax_elem_dtor          (syntax_elem_t** elem);
void           syntax_elem_dtor_recursive(syntax_elem_t** elem);

enum SyntaxError syntaxer_ctor(syntaxer_t* const syntaxer, const lexer_t lexer);
void             syntaxer_dtor(syntaxer_t* const syntaxer);

#endif /* MASIK_SRC_SYNTAXER_FUNCS_FUNCS_H */