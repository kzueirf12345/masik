#ifndef MASIK_BACKEND_SRC_IR_FIST_STRUCTS_H
#define MASIK_BACKEND_SRC_IR_FIST_STRUCTS_H

#include "PYAM_IR/include/libpyam_ir.h"

#define MAX_LABEL_NAME_SIZE (128)

typedef struct IrBlock
{
    enum IrOpBlockType type;
    enum IrOperandType ret_type;
    enum IrOperandType label_type;
    enum IrOperandType operation_type;
    enum IrOperandType operand1_type;
    enum IrOperandType operand2_type;

    size_t ret_num;
    char label_str[MAX_LABEL_NAME_SIZE];
    enum IrOpType operation_num;
    size_t operand1_num;
    size_t operand2_num;
} ir_block_t;


#endif /*MASIK_BACKEND_SRC_IR_FIST_STRUCTS_H*/