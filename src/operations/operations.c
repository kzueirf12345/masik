#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "operations/operations.h"
#include "logger/liblogger.h"
#include "utils/utils.h"


#define OPERATION_HANDLE(num, name_, keyword_, ...)                                                 \
    (operation_t){.type = num,  .keyword = keyword_},

const operation_t OPERATIONS[] =
{
    #include "operations/codegen.h"
};
#undef OPERATION_HANDLE

const size_t OPERATIONS_SIZE = sizeof(OPERATIONS) / sizeof(*OPERATIONS);


#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        if (wcsncmp(keyword_, str, wcslen(keyword_)) == 0) return OP_TYPE_##name_;

enum OpType find_op(const wchar_t* const str)
{
    lassert(!is_invalid_ptr(str), "");

    #include "operations/codegen.h"

    return OP_TYPE_UNKNOWN;
}
#undef OPERATION_HANDLE
