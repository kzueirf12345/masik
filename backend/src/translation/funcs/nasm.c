#include <stdbool.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "translation/funcs/funcs.h"

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

// #define REG_CNT 16

// enum Reg 
// {
//     REG_ZERO       = 0,
//     REG_RAX        = 1,
//     REG_RBX        = 2,
//     REG_RCX        = 3,
//     REG_RDX        = 4,
//     REG_RSI        = 5,
//     REG_RDI        = 6,
//     REG_R8         = 7,
//     REG_R9         = 8,
//     REG_R10        = 9,
//     REG_R11        = 10,
//     REG_R12        = 11,
//     REG_R13        = 12,
//     REG_R14        = 13,
//     REG_R15        = 14,
//     REG_STACK      = 15,
// };
// static_assert(REG_CNT == REG_STACK + 1, "");

// static const char* reg_to_str(const enum Reg reg)
// {
//     switch (reg)
//     {
//     case REG_RAX:  return "rax";
//     case REG_RBX:  return "rbx";
//     case REG_RCX:  return "rcx";
//     case REG_RDX:  return "rdx";
//     case REG_RSI:  return "rsi";
//     case REG_RDI:  return "rdi";
//     case REG_R8:   return "r8";
//     case REG_R9:   return "r9";
//     case REG_R10:  return "r10";
//     case REG_R11:  return "r11";
//     case REG_R12:  return "r12";
//     case REG_R13:  return "r13";
//     case REG_R14:  return "r14";
//     case REG_R15:  return "r15";
        
//     case REG_ZERO:
//     case REG_STACK:
//     default:
//         fprintf(stderr, "Invalid enum for reg_to_str\n");
//         return NULL;
//     }

//     fprintf(stderr, "Invalid enum for reg_to_str\n");
//     return NULL;
// }

typedef struct Func
{
    size_t num;
    size_t count_args;
} func_t;

typedef struct Translator
{
    stack_key_t vars;
    stack_key_t funcs;
    size_t label_num;
    
} translator_t;
 
static enum TranslationError translator_ctor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->funcs, sizeof(func_t), 10));
    translator->label_num = 0;

    return TRANSLATION_ERROR_SUCCESS;
}

static void translator_dtor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    stack_dtor(&translator->vars);
    stack_dtor(&translator->funcs);
    IF_DEBUG(translator->label_num = 0;)
}


#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        static enum TranslationError translate_##name_(translator_t* const translator,              \
                                                       const tree_elem_t* elem, FILE* out);

#include "utils/src/operations/codegen.h"

#undef OPERATION_HANDLE

static enum TranslationError translate_recursive_(translator_t* const translator, 
                                                  const tree_elem_t* elem, 
                                                  FILE* out);


enum TranslationError translate_nasm(const tree_t* const tree, FILE* out)
{
    TREE_VERIFY(tree);
    lassert(!is_invalid_ptr(out), "");

    translator_t translator = {};
    TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));
    fprintf(out, "section .data\n");
    fprintf(out, "HexTable db \"0123456789ABCDEF\"\n\n");
    fprintf(out, "section .text\n");
    fprintf(out, "global _start\n\n");

    fprintf(out, ";;; ---------------------------------------------\n");
    fprintf(out, ";;; Descript:   print num\n");
    fprintf(out, ";;; Entry:      rax  = num\n");
    fprintf(out, ";;;             r11 = base\n");
    fprintf(out, ";;; Exit:       rax = exit code\n");
    fprintf(out, ";;; Destroy:    rcx, rdx, rsi, rdi, r11\n");
    fprintf(out, ";;; ---------------------------------------------\n");
    fprintf(out, "PrintNum:\n");
    fprintf(out, "    mov rdi, rax                            ; rdi - num\n");
    fprintf(out, "\n");
    fprintf(out, "    xor rcx, rcx                            ; rcx - string size\n");
    fprintf(out, "\n");
    fprintf(out, ";;; check to zero and negative\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "js .Negative\n");
    fprintf(out, "jne .Convertion\n");
    fprintf(out, ";;; push '0' in stack\n");
    fprintf(out, "    dec rsp\n");
    fprintf(out, "    mov byte [rsp], '0'\n");
    fprintf(out, "    inc rcx                                 ; ++size\n");
    fprintf(out, "jmp .Print\n");
    fprintf(out, "\n");
    fprintf(out, ".Negative:\n");
    fprintf(out, "    neg rax                                 ; num = -num\n");
    fprintf(out, "\n");
    fprintf(out, ".Convertion:\n");
    fprintf(out, "    xor rdx, rdx                            ; rdx = 0 (in particular edx)\n");
    fprintf(out, "    div r11                                 ; [rax, rdx] = rdx:rax / r11\n");
    fprintf(out, "    mov dl, byte [HexTable + rdx]           ; dl = HexTable[dl]\n");
    fprintf(out, ";;; push dl (digit) in stack\n");
    fprintf(out, "    dec rsp\n");
    fprintf(out, "    mov byte [rsp], dl\n");
    fprintf(out, "\n");
    fprintf(out, "    inc rcx                                 ; ++size\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "jne .Convertion\n");
    fprintf(out, "\n");
    fprintf(out, ";;; check to negative (add '-')\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "jns .Print\n");
    fprintf(out, ";;; push '-' in stack\n");
    fprintf(out, "    dec rsp\n");
    fprintf(out, "    mov byte [rsp], '-'\n");
    fprintf(out, "    inc rcx                                 ; ++size\n");
    fprintf(out, "\n");
    fprintf(out, ".Print:\n");
    fprintf(out, "\n");
    fprintf(out, "    mov rdx, rcx                            ; rdx - size string\n");
    fprintf(out, "    mov rsi, rsp                            ; rsi - addr string for print\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    add rsp, rdx                            ; clean stack (rdx - size string)\n");
    fprintf(out, "    test rax, rax                           ; check error\n");
    fprintf(out, "je .Exit\n");
    fprintf(out, "\n");
    fprintf(out, ".ExitSuccess:\n");
    fprintf(out, "    xor rax, rax                            ; NO ERROR\n");
    fprintf(out, ".Exit:\n");
    fprintf(out, "ret\n\n");

    fprintf(out, "_start:\n");
    fprintf(out, "call main\n");
    fprintf(out, "mov r11, 10\n");
    fprintf(out, "call PrintNum\n");

    fprintf(out, "mov rdi, rax\n");
    fprintf(out, "mov rax, 60\n");
    fprintf(out, "syscall\n\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(&translator, tree->Groot, out),     
                             translator_dtor_(&translator);
    );

    translator_dtor_(&translator);

    return TRANSLATION_ERROR_SUCCESS;
}

#define CHECK_DECLD_VAR_(result_, elem_)                                                            \
    do {                                                                                            \
        if (!(result_ = (size_t*)stack_find(translator->vars, &elem_->lexem.data.var, NULL)))        \
        {                                                                                           \
            fprintf(stderr, "Use undeclarated var with %zu num\n", elem_->lexem.data.var);          \
            return TRANSLATION_ERROR_UNDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)

#define CHECK_UNDECLD_VAR_(elem_)                                                                   \
    do {                                                                                            \
        if (stack_find(translator->vars, &elem_->lexem.data.var, NULL))                             \
        {                                                                                           \
            fprintf(stderr, "Redeclarated var with %zu num\n", elem_->lexem.data.var);              \
            return TRANSLATION_ERROR_REDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)


#define CHECK_DECLD_FUNC_(func_)                                                                    \
    do {                                                                                            \
        if (!(func_t*)stack_find(translator->funcs, &func_, NULL))                                  \
        {                                                                                           \
            fprintf(stderr, "Use undeclarated func with %zu num\n", func_.num);                     \
            return TRANSLATION_ERROR_UNDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)

#define CHECK_UNDECLD_FUNC_(func_)                                                                  \
    do {                                                                                            \
        if (stack_find(translator->vars, &func_, NULL))                                             \
        {                                                                                           \
            fprintf(stderr, "Redeclarated func with %zu num\n", func_.num);                         \
            return TRANSLATION_ERROR_REDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)


#define VAR_IND_(var_)                                                                              \
        (var_ - (size_t*)stack_begin(translator->vars) + 1)

// #define INC_FREE_REG                                                                                \
//         translator->free_reg += (translator->free_reg != REG_STACK)
    
#define USE_LABEL_                                                                                  \
        (translator->label_num++)

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case num_: TRANSLATION_ERROR_HANDLE(translate_##name_(translator, elem, out)); break;
    

enum TranslationError translate_recursive_(translator_t* const translator, const tree_elem_t* elem, 
                                           FILE* out)
{
    if (!elem) return TRANSLATION_ERROR_SUCCESS;

    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    switch (elem->lexem.type)
    {
    case LEXEM_TYPE_NUM:
    {
        IF_DEBUG(fprintf(out, ";;; COMMENT: num\n");)
        fprintf(out, "push qword %ld\n", elem->lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        size_t* finded_var = NULL;
        CHECK_DECLD_VAR_(finded_var, elem);

        IF_DEBUG(fprintf(out, ";;; COMMENT: var\n");)
        fprintf(out, "push qword [rbp-%ld]\n", VAR_IND_(finded_var)*8);
        break;
    }

    case LEXEM_TYPE_OP:
    {
        switch (elem->lexem.data.op)
        {

#include "utils/src/operations/codegen.h"
        
        case OP_TYPE_UNKNOWN:
        default:
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
        break;
    }

    case LEXEM_TYPE_END:
    default:
        return TRANSLATION_ERROR_INVALID_LEXEM_TYPE;
    }

    return TRANSLATION_ERROR_SUCCESS;
}
#undef OPERATION_HANDLE

static enum TranslationError translate_SUM(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: sum\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "add rcx, rbx\n");
    fprintf(out, "push rcx\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUB(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: sub\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "sub rcx, rbx\n");
    fprintf(out, "push rcx\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: mul\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "imul rcx, rbx\n");
    fprintf(out, "push rcx\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: div\n");)
    fprintf(out, "xor rdx, rdx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "pop rax\n");
    fprintf(out, "idiv rcx\n");
    fprintf(out, "push rax\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t pow_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: pow\n");)

    fprintf(out, "pop rcx\n");
    fprintf(out, "pop rdx\n");
    fprintf(out, "mov rbx, 1\n");
    fprintf(out, "test rcx, rcx\n");
    fprintf(out, "je .ZeroPow%zu\n", pow_label_num);

    fprintf(out, ".HelpCycle%zu:\n", pow_label_num);
    fprintf(out, "imul rbx, rdx\n");
    fprintf(out, "loop .HelpCycle%zu\n", pow_label_num);

    fprintf(out, ".ZeroPow%zu:\n", pow_label_num);
    fprintf(out, "push rbx\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: assign\n");)
    fprintf(out, "pop qword [rbp-%ld]\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    CHECK_UNDECLD_VAR_(elem->lt);
    STACK_ERROR_HANDLE_(stack_push(&translator->vars, &elem->lt->lexem.data.var));
    // ++translator->count_var_decl;
    // size_t var_ind = stack_size(translator->vars) - 1;

    if (elem->rt)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    else
    {
        IF_DEBUG(fprintf(out, ";;; COMMENT: decl assign 0\n");)
        fprintf(out, "push 0\n");
    }
    // fprintf(out, "sub rsp, 8\n");
    // fprintf(out, "pop qword [rbp+%zu]\n", var_ind*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IF_DEBUG(fprintf(out, ";;; COMMENT: if\n");)

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    size_t label_else = USE_LABEL_;

    fprintf(out, "pop rbx\n");
    fprintf(out, "test rbx, rbx\n");
    fprintf(out, "je .label%zu\n", label_else);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        IF_DEBUG(fprintf(out, ";;; COMMENT: if->if\n");)
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        size_t label_end = USE_LABEL_;
        fprintf(out, "jmp .label%zu\n", label_end);

        fprintf(out, ".label%zu:\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
        fprintf(out, ".label%zu:\n", label_end);
    }
    else
    {
        IF_DEBUG(fprintf(out, ";;; COMMENT: if->else\n");)
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
        fprintf(out, ".label%zu:\n", label_else);
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    size_t label_condition  = 0;
    size_t label_body       = 0;
    size_t label_else       = 0;
    size_t label_end        = 0;

    IF_DEBUG(fprintf(out, ";;; COMMENT: while\n");)

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
        fprintf(out, "pop rbx\n");
        fprintf(out, "test rbx, rbx\n");
        label_else = USE_LABEL_;
        fprintf(out, "je .label%zu\n", label_else);
        label_body = USE_LABEL_;
        fprintf(out, "jmp .label%zu\n", label_body);
    }

    label_condition = USE_LABEL_;
    fprintf(out, ".label%zu:\n", label_condition);
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    fprintf(out, "pop rbx\n");
    fprintf(out, "test rbx, rbx\n");
    label_end = USE_LABEL_;
    fprintf(out, "je .label%zu\n", label_end);
    IF_DEBUG(fprintf(out, ";;; COMMENT: while->body\n");)
    fprintf(out, ".label%zu:\n", label_body);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    fprintf(out, "jmp .label%zu\n", label_condition);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        IF_DEBUG(fprintf(out, ";;; COMMENT: while->else\n");)
        fprintf(out, ".label%zu:\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
    }

    fprintf(out, ".label%zu:\n", label_end);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t pow_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: pow assign\n");)

    fprintf(out, "pop rcx\n");
    fprintf(out, "mov rbx, qword [rbp-%ld]\n", VAR_IND_(var)*8);
    fprintf(out, "mov rdx, 1\n");
    fprintf(out, "test rcx, rcx\n");
    fprintf(out, "je .ZeroPow%zu\n", pow_label_num);

    fprintf(out, ".HelpCycle%zu:\n", pow_label_num);
    fprintf(out, "imul rdx, rbx\n", VAR_IND_(var)*8);
    fprintf(out, "loop .HelpCycle%zu\n", pow_label_num);

    fprintf(out, ".ZeroPow%zu:\n", pow_label_num);
    fprintf(out, "mov qword [rbp-%ld], rdx\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: sum assign\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "mov rcx, qword [rbp-%ld]\n", VAR_IND_(var)*8);
    fprintf(out, "add rcx, rbx\n");
    fprintf(out, "mov qword [rbp-%ld], rcx\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: sub assign\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "mov rcx, qword [rbp-%ld]\n", VAR_IND_(var)*8);
    fprintf(out, "sub rcx, rbx\n");
    fprintf(out, "mov qword [rbp-%ld], rcx\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: mul assign\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "mov rcx, qword [rbp-%ld]\n", VAR_IND_(var)*8);
    fprintf(out, "imul rcx, rbx\n");
    fprintf(out, "mov qword [rbp-%ld], rcx\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: div assign\n");)
    fprintf(out, "xor rdx, rdx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "mov rax, qword [rbp-%ld]\n", VAR_IND_(var)*8);
    fprintf(out, "idiv rcx\n");
    fprintf(out, "mov qword [rbp-%ld], rax\n", VAR_IND_(var)*8);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: eq\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "jne .NotEq%zu\n", cond_label_num);

    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndEq%zu\n", cond_label_num);

    fprintf(out, ".NotEq%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndEq%zu:\n", cond_label_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));


    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: neq\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "je .NotNeq%zu\n", cond_label_num);
    
    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndNeq%zu\n", cond_label_num);

    fprintf(out, ".NotNeq%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndNeq%zu:\n", cond_label_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: less\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "jge .NotLess%zu\n", cond_label_num);
    
    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndLess%zu\n", cond_label_num);

    fprintf(out, ".NotLess%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndLess%zu:\n", cond_label_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: lesseq\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "jg .NotLesseq%zu\n", cond_label_num);
    
    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndLesseq%zu\n", cond_label_num);

    fprintf(out, ".NotLesseq%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndLesseq%zu:\n", cond_label_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: great\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "jle .NotGreat%zu\n", cond_label_num);
    
    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndGreat%zu\n", cond_label_num);

    fprintf(out, ".NotGreat%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndGreat%zu:\n", cond_label_num);


    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t cond_label_num = USE_LABEL_;

    IF_DEBUG(fprintf(out, ";;; COMMENT: greateq\n");)
    fprintf(out, "pop rbx\n");
    fprintf(out, "pop rcx\n");
    fprintf(out, "cmp rcx, rbx\n");
    fprintf(out, "jl .NotGreateq%zu\n", cond_label_num);
    
    fprintf(out, "push 1\n");
    fprintf(out, "jmp .EndGreateq%zu\n", cond_label_num);

    fprintf(out, ".NotGreateq%zu:\n", cond_label_num);
    fprintf(out, "push 0\n");

    fprintf(out, ".EndGreateq%zu:\n", cond_label_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_FUNC(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
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

    fprintf(out, "\nfunc_%zu_%zu:\n", func.num, func.count_args);
    fprintf(out, "push rbp\n");
    fprintf(out, "mov rbp, rsp\n");

    const tree_elem_t* arg = elem->lt->rt;
    for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    {
        CHECK_UNDECLD_VAR_(arg->rt);
        STACK_ERROR_HANDLE_(stack_push(&translator->vars, &arg->rt->lexem.data.var));
        fprintf(out, "push qword [rbp+%zu]\n", (func.count_args - count)*8);
    }
    if (func.count_args != 0)
    {
        CHECK_UNDECLD_VAR_(arg);
        STACK_ERROR_HANDLE_(stack_push(&translator->vars, &arg->lexem.data.var));
        fprintf(out, "push qword [rsp+%zu]\n", 2*8);
    }

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    // fprintf(out, "RET\n\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {.num = elem->lt->lexem.data.var, .count_args = (elem->rt != NULL)};
    for (tree_elem_t* ptr = elem->rt; ptr != NULL; ptr = ptr->lt)
    {
        func.count_args += (ptr->lexem.type    == LEXEM_TYPE_OP 
                         && ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: call func\n");)
    fprintf(out, "call func_%zu_%zu\n", func.num, func.count_args);
    fprintf(out, "add rsp, %zu\n", func.count_args*8);
    fprintf(out, "push rax\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_MAIN(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    stack_dtor(&translator->vars);
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));

    IF_DEBUG(fprintf(out, ";;; COMMENT: main\n");)
    fprintf(out, "main:\n");
    fprintf(out, "push rbp\n");
    fprintf(out, "mov rbp, rsp\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    // fprintf(out, "RET\n\n");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ARGS_COMMA(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_CALL_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_CALL_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum TranslationError translate_RET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    IF_DEBUG(fprintf(out, ";;; COMMENT: ret\n");)
    fprintf(out, "pop rax\n");
    fprintf(out, "mov rsp, rbp\n");
    fprintf(out, "pop rbp\n");
    fprintf(out, "ret\n");

    return TRANSLATION_ERROR_SUCCESS;
}