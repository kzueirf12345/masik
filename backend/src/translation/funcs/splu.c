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

static enum TranslationError translate_syscall_hlt(FILE* out);
static enum TranslationError translate_syscall_in(FILE* out);
static enum TranslationError translate_syscall_out(FILE* out);
static enum TranslationError translate_syscall_pow(FILE* out);

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

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out(out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow(out));


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

static enum TranslationError translate_syscall_hlt(FILE* out)
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

static enum TranslationError translate_syscall_in(FILE* out)
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

static enum TranslationError translate_syscall_out(FILE* out)
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

static enum TranslationError translate_syscall_pow(FILE* out)
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

/*

static enum TranslationError translate_SUM(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "ADD\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUB(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "SUB\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "MUL\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "DIV\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POW\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    CHECK_UNDECLD_VAR_(elem->lt);
    STACK_ERROR_HANDLE_(stack_push(&translator->vars, &elem->lt->lexem.data.var));
    ++translator->count_var_decl;
    size_t var_ind = stack_size(translator->vars) - 1;

    if (elem->rt)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    else
    {
        fprintf(out, "PUSH 0\n");
    }

    fprintf(out, "POP [%zu+R1]\n", var_ind);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    size_t label_else = USE_LABEL_;

    fprintf(out, "PUSH 0\n");
    fprintf(out, "JE :label%zu\n", label_else);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        size_t label_end = USE_LABEL_;
        fprintf(out, "JMP :label%zu\n", label_end);

        fprintf(out, ":label%zu\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
        fprintf(out, ":label%zu\n", label_end);
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
        fprintf(out, ":label%zu\n", label_else);
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    size_t label_condition  = 0;
    size_t label_body       = 0;
    size_t label_else       = 0;
    size_t label_end        = 0;

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
        fprintf(out, "PUSH 0\n");
        label_else = USE_LABEL_;
        fprintf(out, "JE :label%zu\n", label_else);
        label_body = USE_LABEL_;
        fprintf(out, "JMP :label%zu\n", label_body);
    }

    label_condition = USE_LABEL_;
    fprintf(out, ":label%zu\n", label_condition);
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    fprintf(out, "PUSH 0\n");
    label_end = USE_LABEL_;
    fprintf(out, "JE :label%zu\n", label_end);
    fprintf(out, ":label%zu\n", label_body);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    fprintf(out, "JMP :label%zu\n", label_condition);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        fprintf(out, ":label%zu\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
    }

    fprintf(out, ":label%zu\n", label_end);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld+R1]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POW\n");

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld+R1]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "ADD\n");

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld+R1]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "SUB\n");

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld+R1]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "MUL\n");

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld+R1]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "DIV\n");

    fprintf(out, "POP [%ld+R1]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "EQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "NEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "LESS\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "LEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "GREAT\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "GEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_FUNC(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {.num = elem->lt->lt->lexem.data.var, .count_args = (elem->lt->rt != NULL)};

    for (tree_elem_t* ptr = elem->lt->rt; ptr != NULL; ptr = ptr->lt)
    {
        func.count_args += (ptr->lexem.type    == LEXEM_TYPE_OP 
                         && ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    CHECK_UNDECLD_FUNC_(func);
    STACK_ERROR_HANDLE_(stack_push(&translator->funcs, &func));

    stack_dtor(&translator->vars);
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));
    translator->count_var_decl = 0;

    fprintf(out, ":func_%zu_%zu\n", func.num, func.count_args);

    const tree_elem_t* arg = elem->lt->rt;
    for (size_t count = 0; count < func.count_args - 1; ++count, arg = arg->lt)
    {
        CHECK_UNDECLD_VAR_(arg->rt);
        STACK_ERROR_HANDLE_(stack_push(&translator->vars, &arg->rt->lexem.data.var));
        ++translator->count_var_decl;
        fprintf(out, "POP [%zu+R1]\n", stack_size(translator->vars) - 1);
    }
    if (func.count_args != 0)
    {
        CHECK_UNDECLD_VAR_(arg);
        STACK_ERROR_HANDLE_(stack_push(&translator->vars, &arg->lexem.data.var));
        ++translator->count_var_decl;
        fprintf(out, "POP [%zu+R1]\n", stack_size(translator->vars) - 1);
    }

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    // fprintf(out, "RET\n\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {.num = elem->lt->lexem.data.var, .count_args = (elem->rt != NULL)};
    for (tree_elem_t* ptr = elem->rt; ptr != NULL; ptr = ptr->lt)
    {
        func.count_args += (ptr->lexem.type    == LEXEM_TYPE_OP 
                         && ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    fprintf(out, "PUSH R1\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "PUSH R1\n");
    fprintf(out, "PUSH %zu\n", translator->count_var_decl);
    fprintf(out, "ADD\n");
    fprintf(out, "POP R1\n");

    fprintf(out, "CALL :func_%zu_%zu\n", func.num, func.count_args);

    fprintf(out, "POP R1\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_MAIN(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    stack_dtor(&translator->vars);
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));
    translator->count_var_decl = 0;

    fprintf(out, ":main\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    // fprintf(out, "RET\n\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ARGS_COMMA(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_CALL_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_CALL_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    fprintf(out, "POP R2\n");
    fprintf(out, "POP R1\n");
    fprintf(out, "PUSH R2\n");
    fprintf(out, "PUSH R1\n");

    fprintf(out, "RET\n");

    return TRANSLATION_ERROR_SUCCESS;
}

*/