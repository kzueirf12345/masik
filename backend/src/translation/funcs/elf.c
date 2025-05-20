#include <unistd.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"
#include "translation/structs.h"
#include "elf_map_utils.h"

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


#define STACK_CODE_BEGIN_CAPACITY_ 256
#define SMASH_MAP_SIZE_ 101
static enum TranslationError translator_ctor_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    SMASH_MAP_ERROR_HANDLE_(
        SMASH_MAP_CTOR(
            &translator->labels, 
            SMASH_MAP_SIZE_, 
            sizeof(label_t), 
            sizeof(labels_val_t), 
            func_hash_func_,
            map_key_to_str_,
            map_val_to_str_
        )
    );

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->code, sizeof(uint8_t), STACK_CODE_BEGIN_CAPACITY_));

    translator->cur_block = NULL;

    translator->cur_addr = 0;

    return TRANSLATION_ERROR_SUCCESS;
}
#undef SMASH_MAP_SIZE_

static void translator_dtor_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    smash_map_dtor(&translator->labels); // TODO destruct stacks when processing

    stack_dtor(&translator->code);
}

static enum TranslationError labels_val_ctor_(labels_val_t* const val, const size_t label_addr)
{
    lassert(!is_invalid_ptr(val), "");

    val->label_addr = label_addr;
    STACK_ERROR_HANDLE_(STACK_CTOR(&val->insert_addrs, sizeof(size_t), 1));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError add_not_handle_addr_(elf_translator_t* const translator, 
                                                  const label_t* const label_name, 
                                                  const size_t insert_addr)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(label_name), "");

    labels_val_t* const val = smash_map_get_val(&translator->labels, label_name);

    if (!val)
    {
        TRANSLATION_ERROR_HANDLE(labels_val_ctor_(val, 0));
    }

    STACK_ERROR_HANDLE_(stack_push(&val->insert_addrs, &insert_addr));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_data_(elf_translator_t* const translator, FILE* out);
static enum TranslationError translate_text_(elf_translator_t* const translator, fist_t* const fist, FILE* out);

static enum TranslationError translate_syscall_hlt(elf_translator_t* const translator, FILE* out);
static enum TranslationError translate_syscall_in(elf_translator_t* const translator, FILE* out);
static enum TranslationError translate_syscall_out(elf_translator_t* const translator, FILE* out);
static enum TranslationError translate_syscall_pow(elf_translator_t* const translator, FILE* out);


#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        static enum TranslationError translate_##name_(elf_translator_t* const translator, FILE* out);

#include "PYAM_IR/include/codegen.h"

#undef IR_OP_BLOCK_HANDLE


enum TranslationError translate_elf(const fist_t* const fist, FILE* out)
{
    FIST_VERIFY_ASSERT(fist, NULL);
    lassert(!is_invalid_ptr(out), "");

    const size_t add_offset = 0x10;
    const size_t entry_addr = 0x400000;
    const size_t start_data_addr = 0x410000;

    elf_translator_t translator = {};
    TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));

    translator.cur_addr = entry_addr;

    const size_t headers_size = sizeof(elf_header_t) + add_offset + 2*sizeof(elf_prog_header_t); 

    // skip headers
    static_assert(STACK_CODE_BEGIN_CAPACITY_ > headers_size);
    *stack_size_ptr(translator.code) = headers_size;  

    TRANSLATION_ERROR_HANDLE(
        translate_text_(&translator, fist, out),
        translator_dtor_(&translator);
    );

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
        .p_offset = {.field64 = headers_size}, //TODO
        .p_vaddr = {.field64 = entry_addr},
        .p_paddr = {.field64 = entry_addr},
        .p_filesz = stack_size(translator.code) - headers_size,
        .p_memsz = stack_size(translator.code) - headers_size,
        .p_align = sysconf(_SC_PAGESIZE),                                   // because on linux >= pagesize
    };

    size_t text_size = stack_size(translator.code) - headers_size; //TODO

    TRANSLATION_ERROR_HANDLE(
        translate_data_(&translator, out),
        translator_dtor_(&translator);
    );

    elf_prog_header_t elf_prog_header_data =
    {
        .p_type = 1,                                                         // PT_LOAD
        .p_flags64 = 0x6,                                                    // RW-
        .p_offset = {.field64 = headers_size + text_size}, //TODO
        .p_vaddr = {.field64 = start_data_addr},
        .p_paddr = {.field64 = start_data_addr},
        .p_filesz = stack_size(translator.code) - text_size - headers_size,
        .p_memsz = stack_size(translator.code) - text_size - headers_size,
        .p_align = sysconf(_SC_PAGESIZE),                                   // because on linux >= pagesize
    };

    if (!memcpy(stack_begin(translator.code), &elf_header, sizeof(elf_header)))
    {
        perror("Can't memcpy elf_header in translator.code");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    uint8_t temp[add_offset] = {};
    for (size_t ind = 0; ind < add_offset; ++ind) temp[ind] = 0xFF;

    if (!memcpy(stack_begin(translator.code) + sizeof(elf_header), temp, add_offset))
    {
        perror("Can't memcpy FF after elf_header in translator.code");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (!memcpy(
            stack_begin(translator.code) + sizeof(elf_header) + add_offset, 
            &elf_prog_header_text, 
            sizeof(elf_prog_header_text)
        ))
    {
        perror("Can't memcpy elf_prog_header_text in translator.code");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (!memcpy(
            stack_begin(translator.code) + sizeof(elf_header) + add_offset + sizeof(elf_prog_header_text), 
            &elf_prog_header_data, 
            sizeof(elf_prog_header_data)
        ))
    {
        perror("Can't memcpy elf_prog_header_data in translator.code");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(stack_begin(translator.code), stack_size(translator.code), sizeof(uint8_t), out) 
        != stack_size(translator.code))
    {
        perror("Can't fwrite code in output file");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    return TRANSLATION_ERROR_SUCCESS;
}

enum RegNum
{
    REG_NUM_RAX = 0,
    REG_NUM_RCX = 1,
    REG_NUM_RDX = 2,
    REG_NUM_RBX = 3,
    REG_NUM_RSP = 4,
    REG_NUM_RBP = 5,
    REG_NUM_RSI = 6,
    REG_NUM_RDI = 7,
    REG_NUM_R8  = 8,
    REG_NUM_R9  = 9,
    REG_NUM_R10 = 10,
    REG_NUM_R11 = 11,
    REG_NUM_R12 = 12,
    REG_NUM_R13 = 13,
    REG_NUM_R14 = 14,
    REG_NUM_R15 = 15,
};

enum ModRM
{
    MOD_RM_0_OFF    = 0b00 << 6,
    MOD_RM_8_OFF    = 0b01 << 6,
    MOD_RM_32_OFF   = 0b10 << 6,
    MOD_RM_REG      = 0b11 << 6,
};

enum OpCode
{
    OP_CODE_MOV_RM_R    = 0x89,
    OP_CODE_MOV_R_RM    = 0x8B,
    OP_CODE_MOV_RM_I    = 0xC7,
    OP_CODE_ADD_RM_R    = 0x01,
    OP_CODE_ADD_R_RM    = 0x03,
    OP_CODE_ADD_RM_I    = 0x81,
};

#define WRITE_BYTE_(byte_)                                                                          \
    do {                                                                                            \
        const uint8_t byte = (byte_);                                                               \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, &byte));                                  \
    } while (0)

#define WRITE_WORD_(word_)                                                                          \
    do {                                                                                            \
        const uint16_t word = (word_);                                                              \
        const uint8_t* word_ptr = (uint8_t*)&word;                                                  \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, word_ptr + 0));                           \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, word_ptr + 1));                           \
    } while (0)

#define WRITE_DWORD_(dword_)                                                                        \
    do {                                                                                            \
        const uint32_t dword = (dword_);                                                            \
        const uint8_t* dword_ptr = (uint8_t*)&dword;                                                \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, dword_ptr + 0));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, dword_ptr + 1));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, dword_ptr + 2));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, dword_ptr + 3));                          \
    } while (0)

#define WRITE_QWORD_(qword_)                                                                        \
    do {                                                                                            \
        const uint64_t qword = (qword_);                                                            \
        const uint8_t* qword_ptr = (uint8_t*)&qword;                                                \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 0));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 1));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 2));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 3));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 4));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 5));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 6));                          \
        STACK_ERROR_HANDLE_(stack_push(&translator->code, qword_ptr + 7));                          \
    } while (0)
    
#define REX     (0b01000000)
#define REX_W   (0b01010000)

#define WRITE_CALL_ADDR_(cur_addr, func_addr)                                                       \
        WRITE_BYTE_(0xE8);                                                                          \
        WRITE_DWORD_((uint32_t)((ssize_t)(func_addr) - ((ssize_t)(cur_addr) + 5)))

#define WRITE_CALL_EMPTY_()                                                                         \
        WRITE_BYTE_(0xE8);                                                                          \
        WRITE_DWORD_((uint32_t)0)

#define WRITE_PUSHRM_(mod_rm, reg_num)                                                              \
        WRITE_BYTE_(REX_W + (((reg_num) > 7) << 0));                                                \
        WRITE_BYTE_(0xFF);                                                                          \
        WRITE_BYTE_((mod_rm) + (0x6 << 3) + ((reg_num) % 8))

#define WRITE_PUSHI_(num)                                                                           \
        WRITE_BYTE_(0x68);                                                                          \
        WRITE_DWORD_(num)

#define WRITE_POPRM_(mod_rm, reg_num)                                                               \
        WRITE_BYTE_(REX_W + (((reg_num) > 7) << 0));                                                \
        WRITE_BYTE_(0x8F);                                                                          \
        WRITE_BYTE_((mod_rm) + (0x0 << 3) + ((reg_num) % 8))

#define WRITE_OP_RM_R_(op_code, mod_rm, dest_reg_num, src_reg_num)                                  \
        WRITE_BYTE_(REX_W + (((dest_reg_num) > 7) << 2) + (((src_reg_num) > 7) << 0));              \
        WRITE_BYTE_(op_code);                                                                       \
        WRITE_BYTE_((mod_rm) + (((dest_reg_num) % 8) << 3) + ((src_reg_num) % 8))

#define WRITE_OP_R_RM_(op_code, mod_rm, dest_reg_num, src_reg_num)                                  \
        WRITE_BYTE_(REX_W + (((src_reg_num) > 7) << 2) + (((dest_reg_num) > 7) << 0));              \
        WRITE_BYTE_(op_code);                                                                       \
        WRITE_BYTE_((mod_rm) + (((src_reg_num) % 8) << 3) + ((dest_reg_num) % 8))
    
#define WRITE_OP_RM_I_(op_code, mod_rm, reg_num, imm32)                                             \
        WRITE_BYTE_(REX_W + (((reg_num) > 7) << 0));                                                \
        WRITE_BYTE_(op_code);                                                                       \
        WRITE_BYTE_((mod_rm) + (0 << 3) + (reg_num % 8));                                           \
        WRITE_BYTE_(imm32)

#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        case num_:                                                                                  \
            TRANSLATION_ERROR_HANDLE(                                                               \
                translate_##name_(translator, out)                                                  \
            );                                                                                      \
            break;

static enum TranslationError translate_text_(elf_translator_t* const translator, fist_t* const fist, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    for (size_t elem_ind = fist->next[0]; elem_ind; elem_ind = fist->next[elem_ind])
    {
        translator->cur_block = (const ir_block_t*)fist->data + elem_ind;
        switch (translator->cur_block->type)
        {
    
#include "PYAM_IR/include/codegen.h"
            
        case IR_OP_BLOCK_TYPE_INVALID:
        default:
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
    }

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt(&translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in(&translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out(&translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow(&translator, out));

    return TRANSLATION_ERROR_SUCCESS;
}

#undef IR_OP_BLOCK_HANDLE

static enum TranslationError translate_data_(elf_translator_t* const translator, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

    return TRANSLATION_ERROR_SUCCESS;
}


static enum TranslationError translate_CALL_FUNCTION(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy block->label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    const size_t* func_addr_ptr = smash_map_get_val(&translator->labels, &func);

    if (!func_addr_ptr)
    {
        WRITE_CALL_EMPTY_();
        TRANSLATION_ERROR_HANDLE(add_not_handle_addr_(translator, &func, translator->cur_addr + 1));
    }
    else
    {
        WRITE_CALL_ADDR_(translator->cur_addr, *func_addr_ptr);
    }

    translator->cur_addr += 5; // call size

    WRITE_PUSHR_(REG_NUM_RAX);
    translator->cur_addr += 1;

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNCTION_BODY(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    WRITE_POPRM_(MOD_RM_REG, REG_NUM_RAX); // save ret val
    WRITE_OP_RM_R_(OP_CODE_MOV_RM_R, MOD_RM_REG, REG_NUM_RBX, REG_NUM_RBP); // save old rbp

    // rbp = rsp + arg_cnt
    WRITE_OP_RM_R_(OP_CODE_MOV_RM_R, MOD_RM_REG, REG_NUM_RBP, REG_NUM_RSP);
    WRITE_OP_RM_I_(OP_CODE_ADD_RM_I, MOD_RM_REG, REG_NUM_RBP, 8 * translator->cur_block->operand1_num);
    // TODO сделать поле для opcode который /число 

    // rsp = rbp - local_vars_cnt
    WRITE_MOV_R_R_(false, REG_NUM_RSP, REG_NUM_RBP);
    WRITE_SUB_R_I_(false, REG_NUM_RSP, 8 * translator->cur_block->operand2_num);

    WRITE_PUSHR_(REG_NUM_RAX); // ret addr
    WRITE_PUSHR_(REG_NUM_RBX); // old rbp

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_COND_JUMP(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    if (translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
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

static enum TranslationError translate_ASSIGNMENT(elf_translator_t* const translator,  FILE* out)
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

static enum TranslationError translate_OPERATION(elf_translator_t* const translator,  FILE* out)
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

static enum TranslationError translate_RETURN(elf_translator_t* const translator,  FILE* out)
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

static enum TranslationError translate_LABEL(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "%s:\n", block->label_str);

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SYSCALL(elf_translator_t* const translator,  FILE* out)
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

static enum TranslationError translate_GLOBAL_VARS(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(out), "");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_hlt(elf_translator_t* const translator, FILE* out)
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

static enum TranslationError translate_syscall_in(elf_translator_t* const translator, FILE* out)
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

static enum TranslationError translate_syscall_out(elf_translator_t* const translator, FILE* out)
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

static enum TranslationError translate_syscall_pow(elf_translator_t* const translator, FILE* out)
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