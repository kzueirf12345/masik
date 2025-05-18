#ifndef MASIK_BACKEND_IR_FIST_FUNCS_FUNCS_H
#define MASIK_BACKEND_IR_FIST_FUNCS_FUNCS_H

#include "ir_fist/verification/verification.h"
#include "hash_table/libs/list_on_array/libfist.h"
#include "ir_fist/structs.h"

enum IrFistError ir_block_init(ir_block_t* const block);

enum IrFistError ir_fist_ctor(fist_t* fist, const char* const filename);

#endif /*MASIK_BACKEND_IR_FIST_FUNCS_FUNCS_H*/