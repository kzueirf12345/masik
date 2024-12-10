#ifndef MASIK_SRC_OPERATIONS_OPERATIONS_H
#define MASIK_SRC_OPERATIONS_OPERATIONS_H

#include <stdio.h>
#include <wchar.h>

#define OPERATION_HANDLE(num, name, ...)                                                            \
        OP_TYPE_##name = num,

enum OpType
{
    #include "operations/codegen.h"
    OP_TYPE_UNKNOWN
};

#undef OPERATION_HANDLE

typedef struct Operation
{
    const enum OpType type;

    // const wchar_t* const name;
    const wchar_t* const keyword;
} operation_t;

extern const operation_t OPERATIONS[];
extern const size_t OPERATIONS_SIZE;

enum OpType find_op(const wchar_t* const str);

#endif /* MASIK_SRC_OPERATIONS_OPERATIONS_H  */