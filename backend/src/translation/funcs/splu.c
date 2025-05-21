#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        const enum StackError stack_error_handler = call_func;                                      \
        if (stack_error_handler)                                                                    \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Stack error: %s\n",                               \
                            stack_strerror(stack_error_handler));                                   \
            __VA_ARGS__                                                                             \
            return TRANSLATION_ERROR_STACK;                                                         \
        }                                                                                           \
    } while(0)

static enum TranslationError translate_syscall_hlt_(FILE* out);
static enum TranslationError translate_syscall_in_(FILE* out);
static enum TranslationError translate_syscall_out_(FILE* out);
static enum TranslationError translate_syscall_pow_(FILE* out);

#define CUR_BLOCK_ ((const ir_block_t*)fist->data + elem_ind)

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        static enum TranslationError translate_##name_(const ir_block_t* const block, FILE* out);

#include "PYAM_IR/include/codegen.h"

#undef IR_OP_BLOCK_HANDLE

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        case num_: TRANSLATION_ERROR_HANDLE(translate_##name_(CUR_BLOCK_, out)); break;

enum TranslationError translate_splu(const fist_t* const fist, FILE* out)
{
    FIST_VERIFY_ASSERT(fist, NULL);
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "PUSH 0\n");
    fprintf(out, "POP R1\n"); // rbp 
    fprintf(out, "PUSH 0\n");
    fprintf(out, "POP R2\n"); // rsp

    for (size_t elem_ind = fist->next[0]; elem_ind; elem_ind = fist->next[elem_ind])
    {
        switch (CUR_BLOCK_->type)
        {
    
#include "PYAM_IR/include/codegen.h"
            
        case IR_OP_BLOCK_TYPE_INVALID:
        default:
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
    }

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt_(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in_(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out_(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow_(out));


    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_CALL_FUNCTION(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "CALL :%s\n", block->label_str);
    fprintf(out, // ret val
        "PUSH R3\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNCTION_BODY(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "\n:%s\n", block->label_str);
    // //save ret val
    // fprintf(out, "POP R3\n");

    // // push rbp
    fprintf(out, "PUSH R1\n");

    // rbp = rsp - arg_cnt
    fprintf(out, "PUSH R2\n");
    fprintf(out, "PUSH %zu\n", block->operand1_num);
    fprintf(out, "SUB\n");
    fprintf(out, "POP R1\n");

    // rsp = rbp + local_vars_cnt
    fprintf(out, "PUSH R1\n");
    fprintf(out, "PUSH %zu\n", block->operand2_num);
    fprintf(out, "ADD\n");
    fprintf(out, "POP R2\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_COND_JUMP(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    if (block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        fprintf(out, "PUSH %zu\n", block->operand1_num);
    }

    fprintf(out, "PUSH 1\n");
    fprintf(out, "JE :%s\n\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_VAR)
    {
        fprintf(out, "PUSH [%zu+R1]\n", block->operand1_num);
    }
    else if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        fprintf(out, "PUSH %zu\n", block->operand1_num);
    }
    else if (block->ret_type == IR_OPERAND_TYPE_VAR && block->operand1_type == IR_OPERAND_TYPE_TMP)
    {
        fprintf(out, "POP [%zu+R1]\n", block->ret_num);
    }
    else if (block->ret_type == IR_OPERAND_TYPE_ARG && block->operand1_type == IR_OPERAND_TYPE_TMP)
    {
        fprintf(out, "POP [R2]\n");
        fprintf(out, 
            "PUSH R2\n"
            "PUSH 1\n"
            "ADD\n"
            "POP R2\n"
        );
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_OPERATION(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    switch(block->operation_num)
    {
        case IR_OP_TYPE_SUM:
        {
            fprintf(out, "ADD\n");
            break;
        }
        case IR_OP_TYPE_SUB:
        {
            fprintf(out, "SUB\n");
            break;
        }
        case IR_OP_TYPE_MUL:
        {
            fprintf(out, "MUL\n");
            break;
        }
        case IR_OP_TYPE_DIV:
        {
            fprintf(out, "DIV\n");
            break;
        }
        case IR_OP_TYPE_EQ:
        {
            fprintf(out, "EQ\n");
            break;
        }
        case IR_OP_TYPE_NEQ:
        {
            fprintf(out, "NEQ\n");
            break;
        }
        case IR_OP_TYPE_LESS:
        {
            fprintf(out, "LESS\n");
            break;
        }
        case IR_OP_TYPE_LESSEQ:
        {
            fprintf(out, "LEQ\n");
            break;
        }
        case IR_OP_TYPE_GREAT:
        {
            fprintf(out, "GREAT\n");
            break;
        }
        case IR_OP_TYPE_GREATEQ:
        {
            fprintf(out, "GEQ\n");
            break;
        }
        case IR_OP_TYPE_INVALID_OPERATION:
        default:
        {
            fprintf(stderr, "Invalid IR_OP_TYPE\n");
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_RETURN(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, 
        "PUSH R1\n"
        "POP R2\n"
        "POP R3\n" // ret val
        "POP R1\n" // update rbp
        "RET\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LABEL(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, ":%s\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SYSCALL(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    // // push rbp
    fprintf(out, "PUSH R1\n");

    // rbp = rsp - arg_cnt
    fprintf(out, "\nPUSH R2\n");
    fprintf(out, "PUSH %zu\n", block->operand1_num);
    fprintf(out, "SUB\n");
    fprintf(out, "POP R1\n");

    fprintf(out, "CALL :%s\n", block->label_str);

    fprintf(out, "POP R1\n");

    if (kIR_SYS_CALL_ARRAY[block->operand2_num].HaveRetVal)
    {
        fprintf(out, "PUSH R3\n"); // ret val
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GLOBAL_VARS(const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_hlt_(FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, ":hlt\n");

    fprintf(out, 
        "POP R2\n"
        "PUSH [R1]\n"
        "OUT\n"
        "HLT\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_in_(FILE* out)
{
    lassert(!is_invalid_ptr(out), "");
    
    fprintf(out, 
        ":in\n"
        "IN\n"
        "POP R3\n"
        "RET\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_out_(FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, ":out\n");
    
    fprintf(out, 
        "PUSH [R1]\n"
        "OUT\n"
        "RET\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_pow_(FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, ":pow\n");

    fprintf(out,
        "PUSH [R1]\n"
        "PUSH [1+R1]\n"
        "POW\n"
        "POP R3\n"
        "RET\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}