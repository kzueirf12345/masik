#ifndef MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_LIB_H
#define MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_LIB_H

#include "ir_fist/structs.h"
#include "translation/funcs/elf/structs.h"

#define REX     (0b01000000)
#define REX_W   (0b01001000)
#define REX_R   (0b01000100)
#define REX_X   (0b01000010)
#define REX_B   (0b01000001)

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
    MOD_RM_OFF0     = 0b00,
    MOD_RM_OFF1     = 0b01,
    MOD_RM_OFF4     = 0b10,
    MOD_RM_RR       = 0b11,
};

enum OpCode
{
    OP_CODE_CALL_ADDR   = 0xE8,

    OP_CODE_PUSH_R      = 0x50,
    OP_CODE_PUSH_I      = 0x68,

    OP_CODE_POP_R       = 0x58,

    OP_CODE_MOV_R_R     = 0x89,
    OP_CODE_MOV_R_IRM   = 0x8B,
    OP_CODE_MOV_R_I     = 0xB8,

    OP_CODE_ADD_R_I     = 0x81,

    OP_CODE_SUB_R_I     = 0x81,

    OP_CODE_RET         = 0xC3,

    OP_CODE_SYSCALL1    = 0x0F,
    OP_CODE_SYSCALL2    = 0x05,
};

enum OpCodeModRM
{
    OP_CODE_MOD_ADD_R_I     = 0x0,
    OP_CODE_MOD_SUB_R_I     = 0x5
};

enum SIBScale
{
    SIB_SCALE1 = 0b00,
    SIB_SCALE2 = 0b01,
    SIB_SCALE4 = 0b10,
    SIB_SCALE8 = 0b11
};

#define SIB_NO_INDEX (0b100)
#define MOD_RM_USE_SIB (0b100)

enum TranslationError write_byte_text(elf_translator_t* const translator, const uint8_t byte);
enum TranslationError write_word_text(elf_translator_t* const translator, const uint16_t word);
enum TranslationError write_dword_text(elf_translator_t* const translator, const uint32_t dword);
enum TranslationError write_qword_text(elf_translator_t* const translator, const uint64_t qword);

enum TranslationError write_byte_data(elf_translator_t* const translator, const uint8_t byte);
enum TranslationError write_word_data(elf_translator_t* const translator, const uint16_t word);
enum TranslationError write_dword_data(elf_translator_t* const translator, const uint32_t dword);
enum TranslationError write_qword_data(elf_translator_t* const translator, const uint64_t qword);

enum TranslationError write_call_addr   (elf_translator_t* const translator, const size_t func_addr);

enum TranslationError write_push_r      (elf_translator_t* const translator, const enum RegNum reg);
enum TranslationError write_push_i      (elf_translator_t* const translator, const int64_t imm);

enum TranslationError write_pop_r       (elf_translator_t* const translator, const enum RegNum reg);

enum TranslationError write_mov_r_r     (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);
enum TranslationError write_mov_r_irm   (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2,
                                        const int64_t imm);
enum TranslationError write_mov_r_i     (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);

enum TranslationError write_add_r_i     (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);

enum TranslationError write_sub_r_i     (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);

enum TranslationError write_ret         (elf_translator_t* const translator);
enum TranslationError write_syscall     (elf_translator_t* const translator);

#endif /*MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_LIB_H*/