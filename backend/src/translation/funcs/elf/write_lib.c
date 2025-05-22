#include <unistd.h>
#include <elf.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"
#include "translation/funcs/elf/structs.h"
#include "map_utils.h"
#include "write_lib.h"

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

// static size_t get_imm_size_(const int64_t imm)
// {
//     if (imm == (int8_t)imm)
//         return 1;

//     if (imm == (int16_t)imm)
//         return 2;
        

//     if (imm == (int32_t)imm)
//         return 4;
        
//     return 8;
// }

static uint8_t create_modrm_(const enum ModRM mod, const enum RegNum reg1, const enum RegNum reg2)
{
    return (uint8_t)((mod << 6) + ((reg1 % 8) << 3) + (reg2 % 8));
}

static uint8_t create_sib_(const enum RegNum base_reg)
{
    return (uint8_t)((SIB_SCALE1 << 6) + (SIB_NO_INDEX << 3) + (base_reg % 8));
}

static enum TranslationError write_byte_(stack_key_t stack, const uint8_t byte)
{
    STACK_ERROR_HANDLE_(stack_push(&stack, &byte)); 

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError write_word_(stack_key_t stack, const uint16_t word)
{
    const uint8_t* word_ptr = (const uint8_t*)&word;

    STACK_ERROR_HANDLE_(stack_push(&stack, word_ptr + 0)); 
    STACK_ERROR_HANDLE_(stack_push(&stack, word_ptr + 1)); 

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError write_dword_(stack_key_t stack, const uint32_t dword)
{
    const uint8_t* dword_ptr = (const uint8_t*)&dword;

    STACK_ERROR_HANDLE_(stack_push(&stack, dword_ptr + 0)); 
    STACK_ERROR_HANDLE_(stack_push(&stack, dword_ptr + 1)); 
    STACK_ERROR_HANDLE_(stack_push(&stack, dword_ptr + 2)); 
    STACK_ERROR_HANDLE_(stack_push(&stack, dword_ptr + 3)); 

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError write_qword_(stack_key_t stack, const uint64_t qword)
{
    const uint8_t* qword_ptr = (const uint8_t*)&qword;

    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 0));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 1));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 2));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 3));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 4));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 5));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 6));
    STACK_ERROR_HANDLE_(stack_push(&stack, qword_ptr + 7));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_byte_text(elf_translator_t* const translator, const uint8_t byte)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_byte_(translator->text, byte));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_word_text(elf_translator_t* const translator, const uint16_t word)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_word_(translator->text, word));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_dword_text(elf_translator_t* const translator, const uint32_t dword)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_dword_(translator->text, dword));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_qword_text(elf_translator_t* const translator, const uint64_t qword)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_qword_(translator->text, qword));

    return TRANSLATION_ERROR_SUCCESS;
}


enum TranslationError write_byte_data(elf_translator_t* const translator, const uint8_t byte)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_byte_(translator->data, byte));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_word_data(elf_translator_t* const translator, const uint16_t word)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_word_(translator->data, word));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_dword_data(elf_translator_t* const translator, const uint32_t dword)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_dword_(translator->data, dword));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_qword_data(elf_translator_t* const translator, const uint64_t qword)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_qword_(translator->data, qword));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_arr_text(elf_translator_t* const translator, const uint8_t* const arr, const size_t size)
{
    lassert(!is_invalid_ptr(translator), "");

    for (size_t num = 0; num < size; ++num)
    {
        TRANSLATION_ERROR_HANDLE(write_byte_text(translator, arr[num]));
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError write_command_(
                                            elf_translator_t* const translator,
                                            const enum OpCode opcode,
                                            const uint8_t rex,
                                            const uint8_t modrm, 
                                            const uint8_t sib,
                                            const uint64_t imm,  
                                            const size_t imm_size
                                        ) 
{
    lassert(!is_invalid_ptr(translator), "");

    size_t instr_size = 0;

    if (rex) {
        TRANSLATION_ERROR_HANDLE(write_byte_text(translator, rex));
        ++instr_size;
    }

    TRANSLATION_ERROR_HANDLE(write_byte_text(translator, (uint8_t)opcode));
    ++instr_size;

    if (modrm != 0) {
        TRANSLATION_ERROR_HANDLE(write_byte_text(translator, modrm));
        ++instr_size;
    }

    if (sib != 0) {
        TRANSLATION_ERROR_HANDLE(write_byte_text(translator, sib));
        ++instr_size;
    }


    switch (imm_size) {
        case 0:
            break;
        case sizeof(uint8_t):
            TRANSLATION_ERROR_HANDLE(write_byte_text(translator, (uint8_t)imm));
            instr_size += sizeof(uint8_t);
            break;
        case sizeof(uint16_t):
            TRANSLATION_ERROR_HANDLE(write_word_text(translator, (uint16_t)imm));
            instr_size += sizeof(uint16_t);
            break;
        case sizeof(uint32_t):
            TRANSLATION_ERROR_HANDLE(write_dword_text(translator, (uint32_t)imm));
            instr_size += sizeof(uint32_t);
            break;
        case sizeof(uint64_t):
            TRANSLATION_ERROR_HANDLE(write_qword_text(translator, (uint64_t)imm));
            instr_size += sizeof(uint64_t);
            break;
        default:
            return TRANSLATION_ERROR_INVALID_IMM_SIZE;
    }

    translator->cur_addr += instr_size;

    return TRANSLATION_ERROR_SUCCESS;
}


//E8 cd
enum TranslationError write_call_addr(elf_translator_t* const translator, const size_t func_addr)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_CALL_ADDR, 
            0, 
            0,
            0,
            (uint64_t)(func_addr ? (ssize_t)func_addr - ((ssize_t)translator->cur_addr + 5) : 0),
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

//50+rd
enum TranslationError write_push_r(elf_translator_t* const translator, const enum RegNum reg)
{
    lassert(!is_invalid_ptr(translator), "");

    #pragma GCC diagnostic push 
    #pragma GCC diagnostic ignored "-Wformat=" 

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_PUSH_R + reg % 8, 
            reg > 7 ? REX_W | REX_B : 0, 
            0,
            0,
            0,
            0
        )
    );

    #pragma GCC diagnostic pop

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_push_i(elf_translator_t* const translator, const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_PUSH_I, 
            0,
            0,
            0,
            (uint64_t)imm,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_push_irm(elf_translator_t* const translator, 
                                        const enum RegNum reg, 
                                        const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_PUSH_IRM, 
            REX_W | (reg > 7 ? REX_B : 0),
            create_modrm_(MOD_RM_OFF4, OP_CODE_MOD_PUSH_IRM, reg),
            0,
            (uint64_t)imm,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

//58+ rd
enum TranslationError write_pop_r(elf_translator_t* const translator, const enum RegNum reg)
{
    lassert(!is_invalid_ptr(translator), "");

    #pragma GCC diagnostic push 
    #pragma GCC diagnostic ignored "-Wformat=" 
    
    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_POP_R + reg % 8, 
            reg > 7 ? REX_W | REX_B : 0, 
            0,
            0,
            0,
            0
        )
    );

    #pragma GCC diagnostic pop

    return TRANSLATION_ERROR_SUCCESS;
}

// REX.W + 89 /r
// MOV r/m64, r64
enum TranslationError write_mov_r_r(elf_translator_t* const translator, 
                                    const enum RegNum reg1,
                                    const enum RegNum reg2)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_MOV_R_R,
            REX_W | (reg1 > 7 ? REX_B : 0) | (reg2 > 7 ? REX_R : 0), 
            create_modrm_(MOD_RM_RR, reg2, reg1), /*reverse its correct*/
            0,
            0,
            0
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

// REX.W + 8B /r
enum TranslationError write_mov_r_irm(elf_translator_t* const translator, 
                                      const enum RegNum reg1,
                                      const enum RegNum reg2,
                                      const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_MOV_R_IRM,
            REX_W | (reg1 > 7 ? REX_B : 0) | (reg2 > 7 ? REX_R : 0), 
            create_modrm_(MOD_RM_OFF4, reg1, MOD_RM_USE_SIB),
            create_sib_(reg2),
            (uint64_t)imm,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

//REX.W + B8+ rd io
enum TranslationError write_mov_r_i(elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    #pragma GCC diagnostic push 
    #pragma GCC diagnostic ignored "-Wformat=" 

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_MOV_R_I + reg % 8,
            REX_W | (reg > 7 ? REX_B : 0), 
            0,
            0,
            (uint64_t)imm,
            sizeof(uint64_t)
        )
    );

    #pragma GCC diagnostic pop

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_mov_rm_i8(elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_MOV_RM_I8,
            REX_W | (reg > 7 ? REX_B : 0), 
            create_modrm_(MOD_RM_OFF0, OP_CODE_MOD_MOV_RM_I8, MOD_RM_USE_SIB),
            create_sib_(reg),
            (uint8_t)imm,
            sizeof(uint8_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

// REX.W + 81 /0 id
enum TranslationError write_add_r_i(elf_translator_t* const translator, 
                                    const enum RegNum reg,
                                    const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_ADD_R_I,
            REX_W | (reg > 7 ? REX_B : 0), 
            create_modrm_(MOD_RM_RR, OP_CODE_MOD_ADD_R_I, reg),
            0,
            (uint64_t)imm,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

// REX.W + 81 /5 id
enum TranslationError write_sub_r_i(elf_translator_t* const translator, 
                                    const enum RegNum reg,
                                    const int64_t imm)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_SUB_R_I,
            REX_W | (reg > 7 ? REX_B : 0), 
            create_modrm_(MOD_RM_RR, (const enum RegNum)OP_CODE_MOD_SUB_R_I, reg),
            0,
            (uint64_t)imm,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

//C3
enum TranslationError write_ret(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_RET,
            0, 
            0,
            0,
            0,
            0
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

// 0F 05
enum TranslationError write_syscall(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_byte_text(translator, OP_CODE_SYSCALL1));
    TRANSLATION_ERROR_HANDLE(write_byte_text(translator, OP_CODE_SYSCALL2));

    translator->cur_addr += 2;

    return TRANSLATION_ERROR_SUCCESS;
}


// REX.W + 31 /r
// XOR r/m64, r64
enum TranslationError write_xor_r_r(elf_translator_t* const translator, 
                                    const enum RegNum reg1,
                                    const enum RegNum reg2)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_XOR_R_R,
            REX_W | (reg1 > 7 ? REX_B : 0) | (reg2 > 7 ? REX_R : 0), 
            create_modrm_(MOD_RM_RR, reg2, reg1), /*reverse its correct*/
            0,
            0,
            0
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_dec_r(elf_translator_t* const translator, const enum RegNum reg)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_DEC_R,
            REX_W | (reg > 7 ? REX_B : 0), 
            create_modrm_(MOD_RM_RR, OP_CODE_MOD_DEC_R, reg),
            0,
            0,
            0
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_jmp(elf_translator_t* const translator, const size_t rel_addr)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            OP_CODE_JMP, 
            0, 
            0,
            0,
            (uint64_t)rel_addr,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_cond_jmp(elf_translator_t* const translator, 
                                        const enum OpCode jmp_opcode, 
                                        const size_t rel_addr)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_byte_text(translator, OP_CODE_PREF_JMP));

    ++translator->cur_addr;

    TRANSLATION_ERROR_HANDLE(
        write_command_(
            translator, 
            jmp_opcode, 
            0, 
            0,
            0,
            (uint64_t)rel_addr,
            sizeof(uint32_t)
        )
    );

    return TRANSLATION_ERROR_SUCCESS;
}