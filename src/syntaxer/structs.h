#ifndef MASIK_SRC_SYNTAXER_SYNTAXER
#define MASIK_SRC_SYNTAXER_SYNTAXER

#include "lexer/lexer.h"

typedef struct SyntaxElem
{
    lexem_t lexem;
    struct SyntaxElem* lt;
    struct SyntaxElem* rt;
} syntax_elem_t;

typedef struct Syntaxer
{
    syntax_elem_t* Groot;
    size_t size;
} syntaxer_t;



#endif /* MASIK_SRC_SYNTAXER_SYNTAXER */