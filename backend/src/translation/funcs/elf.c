#include <unistd.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"
#include "translation/structs.h"

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

static enum TranslationError translate_syscall_hlt(stack_key_t code, FILE* out);
static enum TranslationError translate_syscall_in(stack_key_t code, FILE* out);
static enum TranslationError translate_syscall_out(stack_key_t code, FILE* out);
static enum TranslationError translate_syscall_pow(stack_key_t code, FILE* out);

#define CUR_BLOCK_ ((const ir_block_t*)fist->data + elem_ind)

#define WRITE_BYTE_(byte_)                                                                          \
    do {                                                                                            \
        const uint8_t byte = byte_;                                                                 \
        STACK_ERROR_HANDLE_(stack_push(&code, &byte));                                              \
    } while (0)

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        static enum TranslationError translate_##name_(stack_key_t code,                         \ 
                                                       const ir_block_t* const block, FILE* out);

#include "PYAM_IR/include/codegen.h"

#undef IR_OP_BLOCK_HANDLE

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        case num_: TRANSLATION_ERROR_HANDLE(translate_##name_(code, CUR_BLOCK_, out)); break;

enum TranslationError translate_elf(const fist_t* const fist, FILE* out)
{
    FIST_VERIFY_ASSERT(fist, NULL);
    lassert(!is_invalid_ptr(out), "");

    stack_key_t code = 0;
    STACK_ERROR_HANDLE_(STACK_CTOR(&code, sizeof(uint8_t), 100));

    for (uint8_t num = 0; num <= 0x0F; ++num)
    {
        WRITE_BYTE_(num);
    }

    const uint8_t input_buffer_size = 32;
    WRITE_BYTE_(input_buffer_size);
    for (uint8_t ind = 0; ind < input_buffer_size; ++ind)
    {
        WRITE_BYTE_(0);
    }

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

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt(code, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in(code, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out(code, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow(code, out));


    const size_t add_offset = 0x10;
    const size_t entry_addr = 0x400000;
    const size_t start_data_addr = 0x410000;

    elf_header_t elf_header =
    {
        .e_ident = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // magic
        .e_type = 2,                                                          // exec
        .e_mashine = 0x3E,                                                    // AMD x86-64
        .e_version = 1,
        .e_entry = {.field64 = entry_addr},                                   // why not
        .e_phoff = {.field64 = sizeof(elf_header_t) + add_offset},            // for separate headers                        
        .e_shoff = {.field64 = 0},
        .e_flags = 0,
        .e_ehsize = sizeof(elf_header_t),
        .e_phentsize = sizeof(elf_prog_header_t),
        .e_phnum = 2,
        .e_shentsize = 64,
        .e_shnum = 0,
        .e_shstrndx = 0
    }; 

    elf_prog_header_t elf_prog_header_text =
    {
        .p_type = 1,                                                         // PT_LOAD
        .p_flags64 = 0x5,                                                    // R-X
        .p_offset = {.field64 = elf_header.e_phoff.field64 + 2*sizeof(elf_prog_header_t) + add_offset}, //TODO
        .p_vaddr = {.field64 = entry_addr},
        .p_paddr = {.field64 = entry_addr},
        .p_filesz = 0,                                                      // TODO change after
        .p_memsz = 0,                                                       // TODO change after
        .p_align = sysconf(_SC_PAGESIZE),                                   // because on linux >= pagesize
    };

    elf_prog_header_t elf_prog_header_data =
    {
        .p_type = 1,                                                         // PT_LOAD
        .p_flags64 = 0x6,                                                    // RW-
        .p_offset = {.field64 = elf_header.e_phoff.field64 + sizeof(elf_prog_header_t) + add_offset}, //TODO
        .p_vaddr = {.field64 = start_data_addr},
        .p_paddr = {.field64 = start_data_addr},
        .p_filesz = 0,                                                      // TODO change after
        .p_memsz = 0,                                                       // TODO change after
        .p_align = sysconf(_SC_PAGESIZE),                                   // because on linux >= pagesize
    };


    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_CALL_FUNCTION(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "call %s\n", block->label_str);
    fprintf(out, "push rax\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNCTION_BODY(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "\n%s:\n", block->label_str);

    fprintf(out, "pop rax ; save ret val\n");
    fprintf(out, "mov rbx, rbp ; save old rbp\n");

    fprintf(out, "mov rbp, rsp\n");
    fprintf(out, "add rbp, %zu\n ; rbp = rsp + arg_cnt\n", 8*block->operand1_num);

    fprintf(out, "mov rsp, rbp\n");
    fprintf(out, "sub rsp, %zu ; rsp = rbp - local_vars_cnt\n", 8*block->operand2_num);

    fprintf(out, "push rax ; ret val\n");
    fprintf(out, "push rbx ; old rbp\n");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_COND_JUMP(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    if (block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        fprintf(out, "mov rbx, %zu\n", block->operand1_num);
    }
    else
    {
        fprintf(out, "pop rbx\n");
    }

    fprintf(out, "test rbx, rbx\n");
    fprintf(out, "jne %s\n\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_VAR)
    {
        fprintf(out, "push qword [rbp-%zu]\n", 8 * (block->operand1_num + 1));
    }
    else if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        fprintf(out, "push %zu\n", block->operand1_num);
    }
    else if (block->ret_type == IR_OPERAND_TYPE_VAR && block->operand1_type == IR_OPERAND_TYPE_TMP)
    {
        fprintf(out, "pop qword [rbp-%zu]\n", 8 * (block->ret_num + 1));
    }
    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_OPERATION(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    switch(block->operation_num)
    {
        case IR_OP_TYPE_SUM:
        {
            fprintf(out, 
                "pop rcx\n"
                "pop rbx\n"
                "add rbx, rcx\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_SUB:
        {
            fprintf(out, 
                "pop rcx\n"
                "pop rbx\n"
                "sub rbx, rcx\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_MUL:
        {
            fprintf(out, 
                "pop rcx\n"
                "pop rbx\n"
                "imul rbx, rcx\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_DIV:
        {
            fprintf(out, 
                "xor rdx, rdx\n"
                "pop rcx\n"
                "pop rax\n"
                "idiv rcx\n"
                "push rax\n"
            );
            
            break;
        }
        case IR_OP_TYPE_EQ:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "sete bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_NEQ:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "setne bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_LESS:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "setl bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_LESSEQ:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "setle bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_GREAT:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "setg bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
            break;
        }
        case IR_OP_TYPE_GREATEQ:
        {
            fprintf(out,
                "pop rcx\n"
                "pop rbx\n"
                "cmp rbx, rcx\n"
                "setge bl\n"
                "movzx rbx, bl\n"
                "push rbx\n"
            );
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

static enum TranslationError translate_RETURN(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, 
        "pop rax ; save ret val\n"
        "pop rbx ; rbp val\n"
        "pop rcx ; ret addr\n"
        "mov rsp, rbp\n"
        "mov rbp, rbx\n"
        "push rcx\n"
        "ret\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LABEL(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "%s:\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SYSCALL(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "call %s\n", block->label_str);

    if (kIR_SYS_CALL_ARRAY[block->operand2_num].HaveRetVal)
    {
        fprintf(out, "push rax ; ret val\n");
    }

    fprintf(out, "add rsp, %zu\n", 8*block->operand1_num);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_GLOBAL_VARS(stack_key_t code, const ir_block_t* const block, FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_hlt(stack_key_t code, FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, 
        "hlt:\n"
        "mov rdi, [rsp + 8]\n"
        "mov rax, 60\n"
        "syscall\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_in(stack_key_t code, FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out,
        ";;; ---------------------------------------------\n"
        ";;; Descript:   read num\n"
        ";;; Entry:      NONE\n"
        ";;; Exit:       rax = read num (0 if error)\n"
        ";;; Destroy:    rcx, rdx, rsi, rdi, r11\n"
        ";;; ---------------------------------------------\n"
        "in:\n"
        "    mov rsi, InputBuffer                       ; rsi - buffer addr\n"
        "    mov rdx, InputBufferSize                   ; rdx - buffer size\n"\
        "    mov r10, 10                                ; r10 - base\n"
        "\n"
        "    xor rax, rax                               ; sys_read\n"
        "    xor rdi, rdi                               ; stdin\n"
        "    syscall\n"
        "\n"
        "    test rax, rax                              ; check read result\n"
        "    jle .ExitError\n"
        "\n"
        "    mov rcx, rax                               ; rcx - cnt bytes read\n"
        "    dec rcx                                    ; not handle \\n\n"
        "    xor rax, rax                               ; rax - result num\n"
        "    xor rdi, rdi                               ; rdi - sign flag (0 = positive)\n"
        "\n"
        ";;; check first symbol\n"
        "    mov bl, byte [rsi]\n"
        "    cmp bl, '-'\n"
        "    jne .CheckDigit\n"
        "    inc rdi                                    ; set negative flag\n"
        "    inc rsi                                    ; skip '-'\n"
        "    dec rcx\n"
        "    jz .ExitError                              ; only '-' in input\n"
        "\n"
        ".CheckDigit:\n"
        "    mov bl, byte [rsi]\n"
        "    sub bl, '0'\n"
        "    cmp bl, 9\n"
        "    ja .ExitError                                  ; not a digit\n"
        "\n"
        ".ConvertLoop:\n"
        "    mov bl, byte [rsi]\n"
        "    sub bl, '0'                                ; convert to digit\n"
        "    imul rax, r10                              ; rax *= 10\n"
        "    add rax, rbx                               ; rax += digit\n"
        "\n"
        "    inc rsi                                    ; next char\n"
        "    dec rcx\n"
        "    jnz .ConvertLoop\n"
        "\n"
        ";;; apply sign if needed\n"
        "    test rdi, rdi\n"
        "    jz .ExitSuccess\n"
        "    neg rax\n"
        "\n"
        ".ExitSuccess:\n"
        "    ret\n"
        "\n"
        ".ExitError:\n"
        "    xor rax, rax                               ; return 0 if error\n"
        "    ret\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_out(stack_key_t code, FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out,
        ";;; ---------------------------------------------\n"
        ";;; Descript:   print num\n"
        ";;; Entry:      first pushed arg  = num\n"
        ";;; Exit:       rax = exit code\n"
        ";;; Destroy:    rcx, rdx, rsi, rdi, r11\n"
        ";;; ---------------------------------------------\n"
        "out:\n"
        "    mov rax, [rsp + 8]                      ; rax - num\n"
        "    mov r11, 10                             ; r11 - base\n"
        "\n"
        "    mov rdi, rax                            ; rdi - num\n"
        "\n"
        "    xor rcx, rcx                            ; rcx - string size\n"
        ";;; add \\n \n"
        "    dec rsp\n"
        "    mov byte [rsp], `\\n`\n"
        "    inc rcx                                 ; ++size\n"
        "\n"
        ";;; check to zero and negative\n"
        "    test rax, rax\n"
        "js .Negative\n"
        "jne .Convertion\n"
        ";;; push '0' in stack\n"
        "    dec rsp\n"
        "    mov byte [rsp], '0'\n"
        "    inc rcx                                 ; ++size\n"
        "jmp .Print\n"
        "\n"
        ".Negative:\n"
        "    neg rax                                 ; num = -num\n"
        "\n"
        ".Convertion:\n"
        "    xor rdx, rdx                            ; rdx = 0 (in particular edx)\n"
        "    div r11                                 ; [rax, rdx] = rdx:rax / r11\n"
        "    mov dl, byte [HexTable + rdx]           ; dl = HexTable[dl]\n"
        ";;; push dl (digit) in stack\n"
        "    dec rsp\n"
        "    mov byte [rsp], dl\n"
        "\n"
        "    inc rcx                                 ; ++size\n"
        "    test rax, rax\n"
        "jne .Convertion\n"
        "\n"
        ";;; check to negative (add '-')\n"
        "    test rdi, rdi\n"
        "jns .Print\n"
        ";;; push '-' in stack\n"
        "    dec rsp\n"
        "    mov byte [rsp], '-'\n"
        "    inc rcx                                 ; ++size\n"
        "\n"
        ".Print:\n"
        "\n"
        "    mov rdx, rcx                            ; rdx - size string\n"
        "    mov rsi, rsp                            ; rsi - addr string for print\n"
        "    mov rdi, 1\n"
        "    mov rax, 1\n"
        "    syscall\n"
        "    add rsp, rdx                            ; clean stack (rdx - size string)\n"
        "    test rax, rax                           ; check error\n"
        "je .Exit\n"
        "\n"
        ".ExitSuccess:\n"
        "    xor rax, rax                            ; NO ERROR\n"
        ".Exit:\n"
        "ret\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_pow(stack_key_t code, FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out,
        ";;; ---------------------------------------------\n"
        ";;; Descript:   fast power calculation (x^n)\n"
        ";;; Entry:      first pushed arg  = x (base)\n"
        ";;;             second pushed arg = n (exponent)\n"
        ";;; Exit:       rax = res \n"
        ";;; Destroy:    rcx, rdx\n"
        ";;; ---------------------------------------------\n"
        "pow:\n"
        "    mov rcx, [rsp + 8]                      ; rcx - n\n"
        "    mov rax, [rsp + 16]                     ; rax - x\n"
        "    mov rdx, 1                              ; rdx = 1 - result\n"
        "\n"
        ";;; Проверка особых случаев\n"
        "    test rcx, rcx                           ; n == 0\n"
        "    je .done\n"
        "\n"
        "    cmp rax, 1                              ; x == 1\n"
        "    je .done\n"
        "\n"
        "    test rax, rax                           ; x == 0\n"
        "    je .zero_case\n"
        "\n"
        ".pow_loop:\n"
        "    test rcx, 1                             ; check even\n"
        "    jz .even_power\n"
        "    imul rdx, rax                           ; res *= x \n"
        "    dec rcx                                 ; --n\n"
        "    jz .done                                ; n == 0"
        "\n"
        ".even_power:\n"
        "    imul rax, rax                           ; x *= x\n"
        "    shr rcx, 1                              ; n /= 2\n"
        "    jnz .pow_loop                           ; n != 0\n"
        "\n"
        ".done:\n"
        "    mov rax, rdx                            ; rax - ret val\n"
        "    ret\n"
        "\n"
        ".zero_case:\n"
        "    xor rax, rax                            ; rax = 0 - ret val\n"
        "    ret\n\n"
    );

    return TRANSLATION_ERROR_SUCCESS;
}