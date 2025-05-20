#ifndef MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H
#define MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H

#include <stdint.h>

#include "hash_table/libhash_table.h"
#include "stack_on_array/libstack.h"
#include "ir_fist/structs.h"

#define MAX_LABEL_NAME_SIZE 64

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
    stack_key_t code;

    ir_block_t* cur_block;

    size_t cur_addr;
    smash_map_t labels;
} elf_translator_t;

typedef union Elf3264Field
{
    uint64_t field64;
    uint32_t field32;
} elf_32_64_field_t;

typedef struct ElfHeader
{
    uint8_t             e_ident[16];                // general file description
    uint16_t            e_type;                     // file type
    uint16_t            e_mashine;                  // mashine architecthutureteck {fuck this word}
    uint32_t            e_version;                  // always 1
    elf_32_64_field_t   e_entry;                    // entry point addr (_start)
    elf_32_64_field_t   e_phoff;                    // offset programm headers table (0 if not)
    elf_32_64_field_t   e_shoff;                    // offset programm sections table (0 if not)
    uint32_t            e_flags;                    // flags for mashine (0 if not)
    uint16_t            e_ehsize;                   // size this shit (32bit - 52, 64bit - 64)
    uint16_t            e_phentsize;                // programm header size (32bit - 32, 64bit - 56)  
    uint16_t            e_phnum;                    // programm headers count
    uint16_t            e_shentsize;                // programm sections size (32bit - 40, 64bit - 64)                 
    uint16_t            e_shnum;                    // programm sections count
    uint16_t            e_shstrndx;                 // index programm section table with sections names .shstrtab (0 if not)
} elf_header_t;

typedef struct ElfProgHeader{
    uint32_t p_type;                                // segment type
    uint32_t p_flags64;                             // chmod flags on 64bit
    elf_32_64_field_t p_offset;                     // segment offset
    elf_32_64_field_t p_vaddr;                      // virtual addr for mapping segment
    elf_32_64_field_t p_paddr;                      // physical addr for mapping segment (usuall = p_vaddr)
    elf_32_64_field_t p_filesz;                     // segment size in file
    elf_32_64_field_t p_memsz;                      // segment size in mempry
    elf_32_64_field_t p_align;                      // segment align (usuall 4096)
} elf_prog_header_t;

#endif /*MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H*/