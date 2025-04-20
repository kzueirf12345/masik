#ifndef MASIK_UTILS_SRC_OPERATIONS_OP_MATH_H
#define MASIK_UTILS_SRC_OPERATIONS_OP_MATH_H

#include "../tree/structs.h"

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        num_t math_##name_(const num_t first, const num_t second);

#include "codegen.h"

#undef OPERATION_HANDLE

#endif /* MASIK_UTILS_SRC_OPERATIONS_OP_MATH_H */