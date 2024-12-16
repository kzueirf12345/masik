#ifndef MASIK_FRONTED_SRC_LEXER_FUNCS_H
#define MASIK_FRONTED_SRC_LEXER_FUNCS_H

#include <stdio.h>

#include "lexer/structs.h"
#include "lexer/verification/verification.h"

enum LexerError lexer_ctor(lexer_t* const lexer);
void            lexer_dtor(lexer_t* const lexer);
enum LexerError lexer_push(lexer_t* const lexer, const lexem_t lexem);
lexem_t*        lexer_get (lexer_t        lexer, const size_t ind);

enum LexerError lexing(lexer_t* const lexer, const char* const filename);

#endif /* MASIK_FRONTED_SRC_LEXER_FUNCS_H */