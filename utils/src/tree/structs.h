#ifndef MASIK_UTILS_SRC_TREE_STRUCTS_H
#define MASIK_UTILS_SRC_TREE_STRUCTS_H

#include <stdint.h>
#include <assert.h>
#include <wchar.h>

#include "utils/src/utils.h"
#include "utils/src/operations/operations.h"

typedef int64_t num_t;

enum LexemType
{
    LEXEM_TYPE_END = 0,
    LEXEM_TYPE_OP  = 1,
    LEXEM_TYPE_NUM = 2,
    LEXEM_TYPE_VAR = 3,
};
static_assert(LEXEM_TYPE_END == 0);


typedef union LexemData
{
    num_t num;
    enum OpType op;
    size_t var;
} lexem_data_u;

typedef struct Lexem
{
    enum LexemType type;
    lexem_data_u data;
} lexem_t;


typedef struct TreeElem
{
    lexem_t lexem;
    struct TreeElem* lt;
    struct TreeElem* rt;
} tree_elem_t;

typedef struct Tree
{
    tree_elem_t* Groot;
    size_t size;
} tree_t;



#endif /* MASIK_UTILS_SRC_TREE_STRUCTS_H */