#ifndef MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H
#define MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H

#include <stdint.h>
#include <elf.h>

#include "hash_table/libhash_table.h"
#include "stack_on_array/libstack.h"
#include "ir_fist/structs.h"

#define ENTRY_ADDR_     (0x400000)
#define ALIGN_          (0x1000)

typedef struct Label
{
    char name[MAX_LABEL_NAME_SIZE];
} label_t;

typedef struct LabelsVal
{
    size_t label_addr;
    stack_key_t insert_addrs;
} labels_val_t;

typedef struct ElfTranslator
{
    stack_key_t text;
    stack_key_t data;

    size_t HexTable_data_off;
    size_t InputBufferSize_data_off;
    size_t InputBuffer_data_off;

    ir_block_t* cur_block;

    size_t cur_addr;
    smash_map_t labels_map;
    stack_key_t labels_stack;
} elf_translator_t;

typedef struct ElfHeaders
{
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr_text;
    Elf64_Phdr phdr_data;

    const char* shstrtab;
    size_t shstrtab_size;
    Elf64_Shdr shdr_zero;
    Elf64_Shdr shdr_shstrtab;
    Elf64_Shdr shdr_text;
    Elf64_Shdr shdr_data;
} elf_headers_t;

#endif /*MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H*/