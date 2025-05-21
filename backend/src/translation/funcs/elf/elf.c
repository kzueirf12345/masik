#include <unistd.h>
#include <elf.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "../funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"
#include "translation/funcs/elf/structs.h"
#include "map_utils.h"
#include "write_lib.h"
#include "labels.h"
#include "headers.h"

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

#define STACK_DATA_BEGIN_CAPACITY_ 128
#define STACK_CODE_BEGIN_CAPACITY_ 5000
#define SMASH_MAP_SIZE_ 101
static enum TranslationError translator_ctor_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    SMASH_MAP_ERROR_HANDLE_(
        SMASH_MAP_CTOR(
            &translator->labels_map, 
            SMASH_MAP_SIZE_, 
            sizeof(label_t), 
            sizeof(labels_val_t), 
            func_hash_func,
            map_key_to_str,
            map_val_to_str
        )
    );

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->text, sizeof(uint8_t), STACK_CODE_BEGIN_CAPACITY_));
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->data, sizeof(uint8_t), STACK_DATA_BEGIN_CAPACITY_));

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->labels_stack, sizeof(label_t), 1));

    translator->cur_block = NULL;

    translator->cur_addr = ENTRY_ADDR_;

    return TRANSLATION_ERROR_SUCCESS;
}
#undef SMASH_MAP_SIZE_
#undef STACK_CODE_BEGIN_CAPACITY_
#undef STACK_DATA_BEGIN_CAPACITY_

static void translator_dtor_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    for (size_t label_ind = 0; label_ind < stack_size(translator->labels_stack); ++label_ind)
    {
        label_t* label_key = stack_get(translator->labels_stack, label_ind);
        
        labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, label_key);

        stack_dtor(&labels_val->insert_addrs);
    }
    
    smash_map_dtor(&translator->labels_map);
    
    stack_dtor(&translator->labels_stack);
    stack_dtor(&translator->text);
    stack_dtor(&translator->data);
}

static enum TranslationError translate_text_(elf_translator_t* const translator, const fist_t* const fist);
static enum TranslationError translate_data_(elf_translator_t* const translator);

static enum TranslationError translate_syscall_hlt_(elf_translator_t* const translator);
static enum TranslationError translate_syscall_in_(elf_translator_t* const translator);
static enum TranslationError translate_syscall_out_(elf_translator_t* const translator);
static enum TranslationError translate_syscall_pow_(elf_translator_t* const translator);


#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        static enum TranslationError translate_##name_(elf_translator_t* const translator);

#include "PYAM_IR/include/codegen.h"

#undef IR_OP_BLOCK_HANDLE


enum TranslationError translate_elf(const fist_t* const fist, FILE* out)
{
    FIST_VERIFY_ASSERT(fist, NULL);
    lassert(!is_invalid_ptr(out), "");


    elf_translator_t translator = {};
    TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));


    TRANSLATION_ERROR_HANDLE(
        translate_text_(&translator, fist),
        translator_dtor_(&translator);
    );

    TRANSLATION_ERROR_HANDLE(
        translate_data_(&translator),
        translator_dtor_(&translator);
    );

    TRANSLATION_ERROR_HANDLE(
        labels_processing(&translator),
        translator_dtor_(&translator);
    );

    elf_headers_t elf_headers = {};

    TRANSLATION_ERROR_HANDLE(
        elf_headers_ctor(&translator, &elf_headers),
        translator_dtor_(&translator);
    );

    TRANSLATION_ERROR_HANDLE(
        write_elf(&translator, &elf_headers, out),
        translator_dtor_(&translator);
    );

    translator_dtor_(&translator);

    return TRANSLATION_ERROR_SUCCESS;
}

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        case num_:                                                                                  \
            TRANSLATION_ERROR_HANDLE(translate_##name_(translator));                                \
            break;

static enum TranslationError translate_text_(elf_translator_t* const translator, const fist_t* const fist)
{
    lassert(!is_invalid_ptr(translator), "");

    for (size_t elem_ind = fist->next[0]; elem_ind; elem_ind = fist->next[elem_ind])
    {
        translator->cur_block = (ir_block_t*)fist->data + elem_ind;
        switch (translator->cur_block->type)
        {
    
#include "PYAM_IR/include/codegen.h"
            
        case IR_OP_BLOCK_TYPE_INVALID:
        default:
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
    }

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt_(translator));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in_(translator));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out_(translator));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow_(translator));

    return TRANSLATION_ERROR_SUCCESS;
}

#undef IR_OP_BLOCK_HANDLE

#define INPUT_BUFFER_SIZE_ 32u
static enum TranslationError translate_data_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    translator->HexTable_data_off = 0;

    for (uint8_t num = 0; num <= 0xF; ++num)
    {
        TRANSLATION_ERROR_HANDLE(write_byte_data(translator, num));
    }
    
    translator->InputBufferSize_data_off = 0xF + 1;

    TRANSLATION_ERROR_HANDLE(write_word_data(translator, INPUT_BUFFER_SIZE_));

    translator->InputBuffer_data_off = translator->InputBufferSize_data_off + 2;

    static_assert(INPUT_BUFFER_SIZE_ % 8 == 0, "");
    for (size_t num = 0; num < INPUT_BUFFER_SIZE_ / 8; ++num)
    {
        TRANSLATION_ERROR_HANDLE(write_qword_data(translator, 0ull));
    }

    return TRANSLATION_ERROR_SUCCESS;
}
#undef INPUT_BUFFER_SIZE_


static enum TranslationError translate_CALL_FUNCTION(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy block->label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    const labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, &func);

    if (!labels_val)
    {    
        TRANSLATION_ERROR_HANDLE(add_not_handle_addr(translator, &func, translator->cur_addr + 1));
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, 0));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, labels_val->label_addr));
    }

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RAX));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNCTION_BODY(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy translator->cur_block-label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func));

    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RAX)); // save ret addr
    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RBX, REG_NUM_RBP)); // save old rbp

    // rbp = rsp + arg_cnt
    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RBP, REG_NUM_RSP));
    TRANSLATION_ERROR_HANDLE(write_add_r_i(translator, REG_NUM_RBP, 8 * (int64_t)translator->cur_block->operand1_num));

    // rsp = rbp - local_vars_cnt
    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RSP, REG_NUM_RBP));
    TRANSLATION_ERROR_HANDLE(write_sub_r_i(translator, REG_NUM_RSP, 8 * (int64_t)translator->cur_block->operand2_num));

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RAX)); // ret addr
    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RBX)); // old rbp

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_COND_JUMP(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // if (translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
    // {
    //     fprintf(out, "mov rbx, %zu\n", block->operand1_num);
    // }
    // else
    // {
    //     fprintf(out, "pop rbx\n");
    // }

    // fprintf(out, "test rbx, rbx\n");
    // fprintf(out, "jne %s\n\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_VAR)
    // {
    //     fprintf(out, "push qword [rbp-%zu]\n", 8 * (block->operand1_num + 1));
    // }
    if (translator->cur_block->ret_type      == IR_OPERAND_TYPE_TMP 
     && translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        TRANSLATION_ERROR_HANDLE(write_push_i(translator, (int64_t)translator->cur_block->operand1_num));
    }
    // else if (block->ret_type == IR_OPERAND_TYPE_VAR && block->operand1_type == IR_OPERAND_TYPE_TMP)
    // {
    //     fprintf(out, "pop qword [rbp-%zu]\n", 8 * (block->ret_num + 1));
    // }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_OPERATION(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // switch(block->operation_num)
    // {
    //     case IR_OP_TYPE_SUM:
    //     {
    //         fprintf(out, 
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "add rbx, rcx\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_SUB:
    //     {
    //         fprintf(out, 
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "sub rbx, rcx\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_MUL:
    //     {
    //         fprintf(out, 
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "imul rbx, rcx\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_DIV:
    //     {
    //         fprintf(out, 
    //             "xor rdx, rdx\n"
    //             "pop rcx\n"
    //             "pop rax\n"
    //             "idiv rcx\n"
    //             "push rax\n"
    //         );
            
    //         break;
    //     }
    //     case IR_OP_TYPE_EQ:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "sete bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_NEQ:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "setne bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_LESS:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "setl bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_LESSEQ:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "setle bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_GREAT:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "setg bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_GREATEQ:
    //     {
    //         fprintf(out,
    //             "pop rcx\n"
    //             "pop rbx\n"
    //             "cmp rbx, rcx\n"
    //             "setge bl\n"
    //             "movzx rbx, bl\n"
    //             "push rbx\n"
    //         );
    //         break;
    //     }
    //     case IR_OP_TYPE_INVALID_OPERATION:
    //     default:
    //     {
    //         fprintf(stderr, "Invalid IR_OP_TYPE\n");
    //         return TRANSLATION_ERROR_INVALID_OP_TYPE;
    //     }
    // }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_RETURN(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RAX)); // ret val
    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RBX)); // rbp val
    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RCX)); // ret addr

    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RSP, REG_NUM_RBP));
    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RBP, REG_NUM_RBX));

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RCX));
    TRANSLATION_ERROR_HANDLE(write_ret(translator));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LABEL(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");
    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy translator->cur_block-label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SYSCALL(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy block->label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    const labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, &func);

    if (!labels_val)
    {    
        TRANSLATION_ERROR_HANDLE(add_not_handle_addr(translator, &func, translator->cur_addr + 1));
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, 0));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, labels_val->label_addr));
    }


    if (kIR_SYS_CALL_ARRAY[translator->cur_block->operand2_num].HaveRetVal)
    {
        TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RAX)); // ret val
    }

    TRANSLATION_ERROR_HANDLE(write_add_r_i(translator, REG_NUM_RSP, 8 * (int64_t)translator->cur_block->operand1_num));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GLOBAL_VARS(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_hlt_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func = {.name = "hlt"};

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func));

    TRANSLATION_ERROR_HANDLE(write_mov_r_irm(translator, REG_NUM_RDI, REG_NUM_RSP, 8));
    TRANSLATION_ERROR_HANDLE(write_mov_r_i(translator, REG_NUM_RAX, 60));
    TRANSLATION_ERROR_HANDLE(write_syscall(translator));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_in_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // fprintf(out,
    //     ";;; ---------------------------------------------\n"
    //     ";;; Descript:   read num\n"
    //     ";;; Entry:      NONE\n"
    //     ";;; Exit:       rax = read num (0 if error)\n"
    //     ";;; Destroy:    rcx, rdx, rsi, rdi, r11\n"
    //     ";;; ---------------------------------------------\n"
    //     "in:\n"
    //     "    mov rsi, InputBuffer                       ; rsi - buffer addr\n"
    //     "    mov rdx, InputBufferSize                   ; rdx - buffer size\n"
    //     "    mov r10, 10                                ; r10 - base\n"
    //     "\n"
    //     "    xor rax, rax                               ; sys_read\n"
    //     "    xor rdi, rdi                               ; stdin\n"
    //     "    syscall\n"
    //     "\n"
    //     "    test rax, rax                              ; check read result\n"
    //     "    jle .ExitError\n"
    //     "\n"
    //     "    mov rcx, rax                               ; rcx - cnt bytes read\n"
    //     "    dec rcx                                    ; not handle \\n\n"
    //     "    xor rax, rax                               ; rax - result num\n"
    //     "    xor rdi, rdi                               ; rdi - sign flag (0 = positive)\n"
    //     "\n"
    //     ";;; check first symbol\n"
    //     "    mov bl, byte [rsi]\n"
    //     "    cmp bl, '-'\n"
    //     "    jne .CheckDigit\n"
    //     "    inc rdi                                    ; set negative flag\n"
    //     "    inc rsi                                    ; skip '-'\n"
    //     "    dec rcx\n"
    //     "    jz .ExitError                              ; only '-' in input\n"
    //     "\n"
    //     ".CheckDigit:\n"
    //     "    mov bl, byte [rsi]\n"
    //     "    sub bl, '0'\n"
    //     "    cmp bl, 9\n"
    //     "    ja .ExitError                                  ; not a digit\n"
    //     "\n"
    //     ".ConvertLoop:\n"
    //     "    mov bl, byte [rsi]\n"
    //     "    sub bl, '0'                                ; convert to digit\n"
    //     "    imul rax, r10                              ; rax *= 10\n"
    //     "    add rax, rbx                               ; rax += digit\n"
    //     "\n"
    //     "    inc rsi                                    ; next char\n"
    //     "    dec rcx\n"
    //     "    jnz .ConvertLoop\n"
    //     "\n"
    //     ";;; apply sign if needed\n"
    //     "    test rdi, rdi\n"
    //     "    jz .ExitSuccess\n"
    //     "    neg rax\n"
    //     "\n"
    //     ".ExitSuccess:\n"
    //     "    ret\n"
    //     "\n"
    //     ".ExitError:\n"
    //     "    xor rax, rax                               ; return 0 if error\n"
    //     "    ret\n\n"
    // );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_out_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // fprintf(out,
    //     ";;; ---------------------------------------------\n"
    //     ";;; Descript:   print num\n"
    //     ";;; Entry:      first pushed arg  = num\n"
    //     ";;; Exit:       rax = exit code\n"
    //     ";;; Destroy:    rcx, rdx, rsi, rdi, r11\n"
    //     ";;; ---------------------------------------------\n"
    //     "out:\n"
    //     "    mov rax, [rsp + 8]                      ; rax - num\n"
    //     "    mov r11, 10                             ; r11 - base\n"
    //     "\n"
    //     "    mov rdi, rax                            ; rdi - num\n"
    //     "\n"
    //     "    xor rcx, rcx                            ; rcx - string size\n"
    //     ";;; add \\n \n"
    //     "    dec rsp\n"
    //     "    mov byte [rsp], `\\n`\n"
    //     "    inc rcx                                 ; ++size\n"
    //     "\n"
    //     ";;; check to zero and negative\n"
    //     "    test rax, rax\n"
    //     "js .Negative\n"
    //     "jne .Convertion\n"
    //     ";;; push '0' in stack\n"
    //     "    dec rsp\n"
    //     "    mov byte [rsp], '0'\n"
    //     "    inc rcx                                 ; ++size\n"
    //     "jmp .Print\n"
    //     "\n"
    //     ".Negative:\n"
    //     "    neg rax                                 ; num = -num\n"
    //     "\n"
    //     ".Convertion:\n"
    //     "    xor rdx, rdx                            ; rdx = 0 (in particular edx)\n"
    //     "    div r11                                 ; [rax, rdx] = rdx:rax / r11\n"
    //     "    mov dl, byte [HexTable + rdx]           ; dl = HexTable[dl]\n"
    //     ";;; push dl (digit) in stack\n"
    //     "    dec rsp\n"
    //     "    mov byte [rsp], dl\n"
    //     "\n"
    //     "    inc rcx                                 ; ++size\n"
    //     "    test rax, rax\n"
    //     "jne .Convertion\n"
    //     "\n"
    //     ";;; check to negative (add '-')\n"
    //     "    test rdi, rdi\n"
    //     "jns .Print\n"
    //     ";;; push '-' in stack\n"
    //     "    dec rsp\n"
    //     "    mov byte [rsp], '-'\n"
    //     "    inc rcx                                 ; ++size\n"
    //     "\n"
    //     ".Print:\n"
    //     "\n"
    //     "    mov rdx, rcx                            ; rdx - size string\n"
    //     "    mov rsi, rsp                            ; rsi - addr string for print\n"
    //     "    mov rdi, 1\n"
    //     "    mov rax, 1\n"
    //     "    syscall\n"
    //     "    add rsp, rdx                            ; clean stack (rdx - size string)\n"
    //     "    test rax, rax                           ; check error\n"
    //     "je .Exit\n"
    //     "\n"
    //     ".ExitSuccess:\n"
    //     "    xor rax, rax                            ; NO ERROR\n"
    //     ".Exit:\n"
    //     "ret\n\n"
    // );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_pow_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // fprintf(out,
    //     ";;; ---------------------------------------------\n"
    //     ";;; Descript:   fast power calculation (x^n)\n"
    //     ";;; Entry:      first pushed arg  = x (base)\n"
    //     ";;;             second pushed arg = n (exponent)\n"
    //     ";;; Exit:       rax = res \n"
    //     ";;; Destroy:    rcx, rdx\n"
    //     ";;; ---------------------------------------------\n"
    //     "pow:\n"
    //     "    mov rcx, [rsp + 8]                      ; rcx - n\n"
    //     "    mov rax, [rsp + 16]                     ; rax - x\n"
    //     "    mov rdx, 1                              ; rdx = 1 - result\n"
    //     "\n"
    //     ";;; Проверка особых случаев\n"
    //     "    test rcx, rcx                           ; n == 0\n"
    //     "    je .done\n"
    //     "\n"
    //     "    cmp rax, 1                              ; x == 1\n"
    //     "    je .done\n"
    //     "\n"
    //     "    test rax, rax                           ; x == 0\n"
    //     "    je .zero_case\n"
    //     "\n"
    //     ".pow_loop:\n"
    //     "    test rcx, 1                             ; check even\n"
    //     "    jz .even_power\n"
    //     "    imul rdx, rax                           ; res *= x \n"
    //     "    dec rcx                                 ; --n\n"
    //     "    jz .done                                ; n == 0"
    //     "\n"
    //     ".even_power:\n"
    //     "    imul rax, rax                           ; x *= x\n"
    //     "    shr rcx, 1                              ; n /= 2\n"
    //     "    jnz .pow_loop                           ; n != 0\n"
    //     "\n"
    //     ".done:\n"
    //     "    mov rax, rdx                            ; rax - ret val\n"
    //     "    ret\n"
    //     "\n"
    //     ".zero_case:\n"
    //     "    xor rax, rax                            ; rax = 0 - ret val\n"
    //     "    ret\n\n"
    // );

    return TRANSLATION_ERROR_SUCCESS;
}