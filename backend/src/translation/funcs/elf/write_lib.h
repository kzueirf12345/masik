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
    OP_CODE_PUSH_IRM    = 0xFF,

    OP_CODE_POP_R       = 0x58,
    OP_CODE_POP_IRM     = 0x8F,

    OP_CODE_MOV_R_R     = 0x89,
    OP_CODE_MOV_R_IRM   = 0x8B,
    OP_CODE_MOV_R_I     = 0xB8,
    OP_CODE_MOV_RM_I8   = 0xC6,
    // OP_CODE_MOV_R8_IRM  = 0x8A,

    OP_CODE_ADD_R_I     = 0x81,
    OP_CODE_ADD_R_R     = 0x01,

    OP_CODE_SUB_R_I     = 0x81,
    OP_CODE_SUB_R_R     = 0x29,

    OP_CODE_IMUL_R_R_1  = 0x0F,
    OP_CODE_IMUL_R_R_2  = 0xAF,

    OP_CODE_IDIV_R      = 0xF7,

    OP_CODE_RET         = 0xC3,

    OP_CODE_SYSCALL1    = 0x0F,
    OP_CODE_SYSCALL2    = 0x05,

    OP_CODE_XOR_R_R     = 0x31,

    OP_CODE_DEC_R       = 0xFF,


    OP_CODE_JMP         = 0xE9,

    OP_CODE_PREF_JMP    = 0x0F,

    OP_CODE_JA          = 0x87,   
    OP_CODE_JAE         = 0x83,   
    OP_CODE_JB          = 0x82,   
    OP_CODE_JBE         = 0x86,   
    OP_CODE_JC          = 0x82,   
    OP_CODE_JE          = 0x84,   
    OP_CODE_JZ          = 0x84,   
    OP_CODE_JG          = 0x8F,   
    OP_CODE_JGE         = 0x8D,   
    OP_CODE_JL          = 0x8C,   
    OP_CODE_JLE         = 0x8E,   
    OP_CODE_JNA         = 0x86,   
    OP_CODE_JNAE        = 0x82,   
    OP_CODE_JNB         = 0x83,   
    OP_CODE_JNBE        = 0x87,   
    OP_CODE_JNC         = 0x83,   
    OP_CODE_JNE         = 0x85,   
    OP_CODE_JNG         = 0x8E,   
    OP_CODE_JNGE        = 0x8C,   
    OP_CODE_JNL         = 0x8D,   
    OP_CODE_JNLE        = 0x8F,   
    OP_CODE_JNO         = 0x81,   
    OP_CODE_JNP         = 0x8B,   
    OP_CODE_JNS         = 0x89,   
    OP_CODE_JNZ         = 0x85,   
    OP_CODE_JO          = 0x80,   
    OP_CODE_JP          = 0x8A,   
    OP_CODE_JPE         = 0x8A,   
    OP_CODE_JPO         = 0x8B,   
    OP_CODE_JS          = 0x88,   

    OP_CODE_TEST_R_R    = 0x85,

    OP_CODE_CMP_R_R     = 0x39,


    OP_CODE_PREF_SET    = 0x0F,

    OP_CODE_SETE        = 0x94, 
    OP_CODE_SETNE       = 0x95, 
    OP_CODE_SETL        = 0x9C, 
    OP_CODE_SETLE       = 0x9E, 
    OP_CODE_SETG        = 0x9F, 
    OP_CODE_SETGE       = 0x9D, 

    OP_CODE_PREF_MOVZX  = 0x0F,
    OP_CODE_MOVZX       = 0xB6,
};

enum OpCodeModRM
{
    OP_CODE_MOD_ADD_R_I     = 0x0,

    OP_CODE_MOD_SUB_R_I     = 0x5,

    OP_CODE_MOD_DEC_R       = 0x1,

    OP_CODE_MOD_MOV_RM_I8   = 0x0,

    OP_CODE_MOD_PUSH_IRM    = 0x6,

    OP_CODE_MOD_POP_IRM     = 0x0,

    OP_CODE_MOD_IDIV_R      = 0x7,

    OP_CODE_MOD_SET         = 0x0,
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

enum TranslationError write_arr_text(elf_translator_t* const translator, const uint8_t* const arr, const size_t size);

enum TranslationError write_call_addr   (elf_translator_t* const translator, const size_t func_addr);

enum TranslationError write_push_r      (elf_translator_t* const translator, const enum RegNum reg);
enum TranslationError write_push_i      (elf_translator_t* const translator, const int64_t imm);
enum TranslationError write_push_irm    (elf_translator_t* const translator, 
                                        const enum RegNum reg, 
                                        const int64_t imm);

enum TranslationError write_pop_r       (elf_translator_t* const translator, const enum RegNum reg);
enum TranslationError write_pop_irm     (elf_translator_t* const translator, 
                                        const enum RegNum reg, 
                                        const int64_t imm);

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
enum TranslationError write_mov_rm_i8   (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);

enum TranslationError write_add_r_i     (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);
enum TranslationError write_add_r_r     (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_sub_r_i     (elf_translator_t* const translator, 
                                        const enum RegNum reg,
                                        const int64_t imm);
enum TranslationError write_sub_r_r     (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_imul_r_r    (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_idiv_r       (elf_translator_t* const translator, const enum RegNum reg);

enum TranslationError write_ret         (elf_translator_t* const translator);
enum TranslationError write_syscall     (elf_translator_t* const translator);

enum TranslationError write_xor_r_r     (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_dec_r       (elf_translator_t* const translator, const enum RegNum reg);


enum TranslationError write_jmp         (elf_translator_t* const translator, const size_t func_addr);
enum TranslationError write_cond_jmp    (elf_translator_t* const translator, 
                                        const enum OpCode jmp_opcode, 
                                        const size_t func_addr);

enum TranslationError write_cond_set    (elf_translator_t* const translator, 
                                        const enum OpCode set_opcode, 
                                        const enum RegNum reg);

enum TranslationError write_test_r_r    (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_cmp_r_r    (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);

enum TranslationError write_movzx      (elf_translator_t* const translator, 
                                        const enum RegNum reg1,
                                        const enum RegNum reg2);


#endif /*MASIK_BACKEND_SRC_TRANSLATION_FUNCS_ELF_LIB_H*/