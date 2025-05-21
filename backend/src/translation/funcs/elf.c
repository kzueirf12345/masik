#include <unistd.h>
#include <elf.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"
#include "translation/structs.h"
#include "elf_map_utils.h"
#include "elf_lib.h"

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

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->code, sizeof(uint8_t), STACK_CODE_BEGIN_CAPACITY_));

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->labels_stack, sizeof(label_t), 1));

    translator->cur_block = NULL;

    translator->cur_addr = 0;

    return TRANSLATION_ERROR_SUCCESS;
}
#undef SMASH_MAP_SIZE_

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
                                                  label_t* const label_name, 
                                                  const size_t insert_addr)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(label_name), "");

    labels_val_t* const val = smash_map_get_val(&translator->labels_map, label_name);

    
    if (!val)
    {
        labels_val_t new_val = {};
        TRANSLATION_ERROR_HANDLE(labels_val_ctor_(&new_val, 0));

        STACK_ERROR_HANDLE_(stack_push(&new_val.insert_addrs, &insert_addr));

        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->labels_map, 
                (smash_map_elem_t){.key = label_name, .val = &new_val}
            )
        );

        STACK_ERROR_HANDLE_(stack_push(&translator->labels_stack, label_name));
    }
    else
    {
        STACK_ERROR_HANDLE_(stack_push(&val->insert_addrs, &insert_addr));
    }



    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError add_label_(elf_translator_t* const translator, label_t* const label_name)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(label_name), "");
    

    labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, label_name);

    if (!labels_val)
    {
        labels_val_t new_val = {};
        TRANSLATION_ERROR_HANDLE(labels_val_ctor_(&new_val, 0));

        new_val.label_addr = translator->cur_addr;
        
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->labels_map, 
                (smash_map_elem_t){.key = label_name, .val = &new_val}
            )
        );

        STACK_ERROR_HANDLE_(stack_push(&translator->labels_stack, label_name));
    }
    else
    {
        labels_val->label_addr = translator->cur_addr;
    }


    return TRANSLATION_ERROR_SUCCESS;
}

// static enum TranslationError translate_data_(elf_translator_t* const translator, FILE* out);
static enum TranslationError translate_text_(elf_translator_t* const translator, const fist_t* const fist, FILE* out);

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

    const size_t entry_addr = 0x400000;
    const unsigned long align = 0x1000;

    elf_translator_t translator = {};
    TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));

    translator.cur_addr = entry_addr;

    // skip headers
    // static_assert(STACK_CODE_BEGIN_CAPACITY_ > align);
    // *stack_size_ptr(translator.code) = align;  

    TRANSLATION_ERROR_HANDLE(
        translate_text_(&translator, fist, out),
        translator_dtor_(&translator);
    );

    for (size_t label_ind = 0; label_ind < stack_size(translator.labels_stack); ++label_ind)
    {
        label_t* label_key = stack_get(translator.labels_stack, label_ind);
        
        labels_val_t* labels_val = smash_map_get_val(&translator.labels_map, label_key);

        for (size_t insert_addr_ind = 0; insert_addr_ind < stack_size(labels_val->insert_addrs); ++insert_addr_ind)
        {
            size_t* insert_addr = stack_get(labels_val->insert_addrs, insert_addr_ind);
            
            // fprintf(stderr, RED_TEXT("labels_name: %s\n"), label_key->name);
            // fprintf(stderr, RED_TEXT("insert_addr: %x\n"), *insert_addr);
            // fprintf(stderr, RED_TEXT("label_addr: %x\n"), labels_val->label_addr);
            // fprintf(stderr, RED_TEXT("rel_addr: %x\n"), labels_val->label_addr - *insert_addr - 4);

            uint8_t* insert_place = stack_get(translator.code, *insert_addr - entry_addr); 

            size_t rel_addr = labels_val->label_addr - *insert_addr - 4;

            if (!memcpy(insert_place, (uint8_t*)&rel_addr, sizeof(uint32_t)))
            {
                translator_dtor_(&translator);
                perror("Can't memcpy label_addr in insert_addr");
                return TRANSLATION_ERROR_SUCCESS;
            }
        }
    }

    const char shstrtab[] = 
    "\0"
    ".shstrtab\0"
    ".text"; 

    const size_t shstrtab_size = sizeof(shstrtab);

    const size_t text_size = stack_size(translator.code);

    // with section headers info
    // Elf64_Ehdr elf_header =
    // {
    //     .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0},
    //     .e_type = ET_EXEC,
    //     .e_machine = EM_X86_64,
    //     .e_version = EV_CURRENT,
    //     .e_entry = entry_addr,
    //     .e_phoff = sizeof(Elf64_Ehdr),              
    //     .e_shoff = align + text_size + shstrtab_size + align - (text_size + shstrtab_size) % align,
    //     .e_flags = 0,
    //     .e_ehsize = sizeof(Elf64_Ehdr),
    //     .e_phentsize = sizeof(Elf64_Phdr),
    //     .e_phnum = 2,
    //     .e_shentsize = sizeof(Elf64_Shdr),
    //     .e_shnum = 3,
    //     .e_shstrndx = 1,
    // }; 

    Elf64_Ehdr elf_header =
    {
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0},
        .e_type = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,
        .e_entry = entry_addr,
        .e_phoff = sizeof(Elf64_Ehdr),              
        .e_shoff = 0, //align + text_size + align - text_size % align
        .e_flags = 0,
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum = 2,
        .e_shentsize = 0, //sizeof(Elf64_Shdr)
        .e_shnum = 0, //3
        .e_shstrndx = 0, //1
    }; 

    Elf64_Phdr elf_prog_header_text =
    {
        .p_type = PT_LOAD,
        .p_flags = PF_X | PF_R,
        .p_offset = align,
        .p_vaddr = elf_header.e_entry,
        .p_paddr = elf_header.e_entry,
        .p_filesz = text_size,
        .p_memsz = text_size,
        .p_align = align,
    };

    // const size_t headers_size = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + 3*sizeof(Elf64_Shdr); 

    // Elf64_Shdr section_headers[] = {
    //     // Null section
    //     {
    //         .sh_name = 0,
    //         .sh_type = SHT_NULL,
    //         .sh_flags = 0,
    //         .sh_addr = 0,
    //         .sh_offset = 0,
    //         .sh_size = 0,
    //         .sh_link = SHN_UNDEF,
    //         .sh_info = 0,
    //         .sh_addralign = 0,
    //         .sh_entsize = 0
    //     },
        
    //     // .shstrtab
    //     {
    //         .sh_name = 1,
    //         .sh_type = SHT_STRTAB,
    //         .sh_flags = SHF_STRINGS,
    //         .sh_addr = 0,
    //         .sh_offset = elf_header.e_shoff + sizeof(Elf64_Shdr),
    //         .sh_size = shstrtab_size,
    //         .sh_link = 0,
    //         .sh_info = 0,
    //         .sh_addralign = 1,
    //         .sh_entsize = 0
    //     },
        
    //     // .text
    //     {
    //         .sh_name = 11,
    //         .sh_type = SHT_PROGBITS,
    //         .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
    //         .sh_addr = entry_addr,
    //         .sh_offset = align,
    //         .sh_size = text_size,
    //         .sh_link = 0,
    //         .sh_info = 0,
    //         .sh_addralign = align,
    //         .sh_entsize = 0
    //     }
    // };

    if(fwrite(&elf_header, sizeof(elf_header), 1, out) != 1)
    {
        translator_dtor_(&translator);
        perror("Can't fwrite elf_header in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if(fwrite(&elf_prog_header_text, sizeof(elf_prog_header_text), 1, out) != 1)
    {
        translator_dtor_(&translator);
        perror("Can't fwrite elf_prog_header_text in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    uint8_t* align_zero_arr = calloc(align, sizeof(*align_zero_arr));

    const size_t first_align_zero_cnt = align - sizeof(elf_header) - sizeof(elf_prog_header_text);

    if(fwrite(align_zero_arr, sizeof(*align_zero_arr), first_align_zero_cnt, out) 
       != first_align_zero_cnt)
    {
        translator_dtor_(&translator);
        perror("Can't fwrite first align_zero_arr in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(stack_begin(translator.code), sizeof(uint8_t), stack_size(translator.code), out)
        < stack_size(translator.code))
    {
        translator_dtor_(&translator);
        perror("Can't fwrite .text code in output file");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    free(align_zero_arr);



    // const size_t section_headers_size = 3;

    // if (!memcpy(stack_begin(translator.code), &elf_header, sizeof(elf_header)))
    // {
    //     translator_dtor_(&translator);
    //     perror("Can't memcpy elf_header in translator.code");
    //     return TRANSLATION_ERROR_STANDARD_ERRNO;
    // }

    // if (!memcpy(
    //         (uint8_t*)stack_begin(translator.code) + sizeof(elf_header), 
    //         &elf_prog_header_text, 
    //         sizeof(elf_prog_header_text)
    //     ))
    // {
    //     translator_dtor_(&translator);
    //     perror("Can't memcpy elf_prog_header_text in translator.code");
    //     return TRANSLATION_ERROR_STANDARD_ERRNO;
    // }

    // for (size_t section_header_ind = 0; section_header_ind < section_headers_size; ++section_header_ind)
    // {
    //     if (!memcpy(
    //             (uint8_t*)stack_begin(translator.code) 
    //                 + sizeof(elf_header) 
    //                 + sizeof(elf_prog_header_text) 
    //                 + section_header_ind * sizeof(section_headers[section_header_ind]), 
    //             &section_headers[section_header_ind], 
    //             sizeof(section_headers[section_header_ind])
    //         ))
    //     {
    //         translator_dtor_(&translator);
    //         perror("Can't memcpy section_header in translator.code");
    //         return TRANSLATION_ERROR_STANDARD_ERRNO;
    //     }
    // }

    // if (!memcpy(
    //         (uint8_t*)stack_begin(translator.code) + headers_size, 
    //         shstrtab, 
    //         shstrtab_size
    //     ))
    // {
    //     translator_dtor_(&translator);
    //     perror("Can't memcpy shstrtab in translator.code");
    //     return TRANSLATION_ERROR_STANDARD_ERRNO;
    // }

    // if (fwrite(stack_begin(translator.code), sizeof(uint8_t), stack_size(translator.code), out)
    //     < stack_size(translator.code))
    // {
    //     translator_dtor_(&translator);
    //     perror("Can't fwrite code in output file");
    //     return TRANSLATION_ERROR_STANDARD_ERRNO;
    // }

    translator_dtor_(&translator);

    return TRANSLATION_ERROR_SUCCESS;
}



#define IR_OP_BLOCK_HANDLE(num_, name_, ...)                                                        \
        case num_:                                                                                  \
            TRANSLATION_ERROR_HANDLE(                                                               \
                translate_##name_(translator, out)                                                  \
            );                                                                                      \
            break;

static enum TranslationError translate_text_(elf_translator_t* const translator, const fist_t* const fist, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

    TRANSLATION_ERROR_HANDLE(translate_syscall_hlt(translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_in(translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_out(translator, out));
    TRANSLATION_ERROR_HANDLE(translate_syscall_pow(translator, out));

    return TRANSLATION_ERROR_SUCCESS;
}

#undef IR_OP_BLOCK_HANDLE

// static enum TranslationError translate_data_(elf_translator_t* const translator, FILE* out)
// {
//     lassert(!is_invalid_ptr(translator), "");
//     lassert(!is_invalid_ptr(out), "");
//     for (uint8_t num = 0; num <= 0x0F; ++num)
//     {
//         WRITE_BYTE_(num);
//     }
//     const uint8_t input_buffer_size = 32;
//     WRITE_BYTE_(input_buffer_size);
//     for (uint8_t ind = 0; ind < input_buffer_size; ++ind)
//     {
//         WRITE_BYTE_(0);
//     }
//     return TRANSLATION_ERROR_SUCCESS;
// }


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

    const labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, &func);

    if (!labels_val)
    {    
        TRANSLATION_ERROR_HANDLE(add_not_handle_addr_(translator, &func, translator->cur_addr + 1));
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, 0));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(write_call_addr(translator, labels_val->label_addr));
    }

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RAX));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_FUNCTION_BODY(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy translator->cur_block-label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    TRANSLATION_ERROR_HANDLE(add_label_(translator, &func));

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

static enum TranslationError translate_COND_JUMP(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

static enum TranslationError translate_ASSIGNMENT(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    // if (block->ret_type == IR_OPERAND_TYPE_TMP && block->operand1_type == IR_OPERAND_TYPE_VAR)
    // {
    //     fprintf(out, "push qword [rbp-%zu]\n", 8 * (block->operand1_num + 1));
    // }
    if (translator->cur_block->ret_type      == IR_OPERAND_TYPE_TMP 
     && translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        TRANSLATION_ERROR_HANDLE(write_push_i(translator, translator->cur_block->operand1_num));
    }
    // else if (block->ret_type == IR_OPERAND_TYPE_VAR && block->operand1_type == IR_OPERAND_TYPE_TMP)
    // {
    //     fprintf(out, "pop qword [rbp-%zu]\n", 8 * (block->ret_num + 1));
    // }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_OPERATION(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

static enum TranslationError translate_RETURN(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RAX)); // ret val
    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RBX)); // rbp val
    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RCX)); // ret addr

    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RSP, REG_NUM_RBP));
    TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RBP, REG_NUM_RBX));

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RCX));
    TRANSLATION_ERROR_HANDLE(write_ret(translator));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_LABEL(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy translator->cur_block-label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    TRANSLATION_ERROR_HANDLE(add_label_(translator, &func));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_SYSCALL(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy block->label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    const labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, &func);

    if (!labels_val)
    {    
        TRANSLATION_ERROR_HANDLE(add_not_handle_addr_(translator, &func, translator->cur_addr + 1));
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

static enum TranslationError translate_GLOBAL_VARS(elf_translator_t* const translator,  FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_hlt(elf_translator_t* const translator, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

    label_t func = {.name = "hlt"};
    // if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    // {
    //     perror("Can't strncpy block->label_str in func.name");
    //     return TRANSLATION_ERROR_STANDARD_ERRNO;
    // }

    // fprintf(stderr, "hlt label_name: %s\n", func.name);

    TRANSLATION_ERROR_HANDLE(add_label_(translator, &func));

    TRANSLATION_ERROR_HANDLE(write_mov_r_irm(translator, REG_NUM_RDI, REG_NUM_RSP, 8));
    TRANSLATION_ERROR_HANDLE(write_mov_r_i(translator, REG_NUM_RAX, 60));
    TRANSLATION_ERROR_HANDLE(write_syscall(translator));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_in(elf_translator_t* const translator, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

static enum TranslationError translate_syscall_out(elf_translator_t* const translator, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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

static enum TranslationError translate_syscall_pow(elf_translator_t* const translator, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(out), "");

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