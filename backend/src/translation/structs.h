#ifndef MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H
#define MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H

#include <stdint.h>

#include "hash_table/libhash_table.h"
#include "stack_on_array/libstack.h"
#include "ir_fist/structs.h"

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
    smash_map_t labels_map;
    stack_key_t labels_stack;
} elf_translator_t;

typedef union Elf3264Field
{
    uint64_t field64;
    uint32_t field32;
} elf_32_64_field_t;

#endif /*MASIK_BACKEND_SRC_TRANSLATION_STRUCTS_H*/