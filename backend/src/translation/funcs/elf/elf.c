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

    translator->cur_addr += ALIGN_ - translator->cur_addr % ALIGN_;

    return TRANSLATION_ERROR_SUCCESS;
}

#undef IR_OP_BLOCK_HANDLE

#define INPUT_BUFFER_SIZE_ 32u
static enum TranslationError translate_data_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    // label_t func = {.name = "HexTable"};
    // TRANSLATION_ERROR_HANDLE(add_label(translator, &func));
    
    TRANSLATION_ERROR_HANDLE(write_dword_data(translator, INPUT_BUFFER_SIZE_));

    static_assert(INPUT_BUFFER_SIZE_ % 4 == 0, "");
    for (size_t num = 0; num < INPUT_BUFFER_SIZE_ / 4; ++num)
    {
        TRANSLATION_ERROR_HANDLE(write_dword_data(translator, 0ul));
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

    // fprintf(stderr, "func_name: %s\n", func.name);
    TRANSLATION_ERROR_HANDLE(add_not_handle_addr(translator, &func, translator->cur_addr + 1));
    TRANSLATION_ERROR_HANDLE(write_call_addr(translator, 0));

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

    if (translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        TRANSLATION_ERROR_HANDLE(write_mov_r_i(translator, REG_NUM_RBX, translator->cur_block->operand1_num));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RBX));
    }

    TRANSLATION_ERROR_HANDLE(write_test_r_r(translator, REG_NUM_RBX, REG_NUM_RBX));

    label_t func = {};
    if (!strncpy(func.name, translator->cur_block->label_str, sizeof(func.name)))
    {
        perror("Can't strncpy block->label_str in func.name");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    TRANSLATION_ERROR_HANDLE(add_not_handle_addr(translator, &func, translator->cur_addr + 2));
    TRANSLATION_ERROR_HANDLE(write_cond_jmp(translator, OP_CODE_JNE, 0));

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_ASSIGNMENT(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    if (translator->cur_block->ret_type      == IR_OPERAND_TYPE_TMP 
     && translator->cur_block->operand1_type == IR_OPERAND_TYPE_VAR)
    {
        TRANSLATION_ERROR_HANDLE(
            write_push_irm(translator, REG_NUM_RBP, -8 * (translator->cur_block->operand1_num + 1))
        );
    }
    else if (translator->cur_block->ret_type      == IR_OPERAND_TYPE_TMP 
          && translator->cur_block->operand1_type == IR_OPERAND_TYPE_NUM)
    {
        TRANSLATION_ERROR_HANDLE(write_push_i(translator, (int64_t)translator->cur_block->operand1_num));
    }
    else if (translator->cur_block->ret_type      == IR_OPERAND_TYPE_VAR 
          && translator->cur_block->operand1_type == IR_OPERAND_TYPE_TMP)
    {
        TRANSLATION_ERROR_HANDLE(
            write_pop_irm(translator, REG_NUM_RBP, -8 * (translator->cur_block->ret_num + 1))
        );
    }

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_OPERATION(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RCX));
    TRANSLATION_ERROR_HANDLE(write_pop_r(translator, REG_NUM_RBX));

    switch(translator->cur_block->operation_num)
    {
        case IR_OP_TYPE_SUM:
        {
            TRANSLATION_ERROR_HANDLE(write_add_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            break;
        }
        case IR_OP_TYPE_SUB:
        {
            TRANSLATION_ERROR_HANDLE(write_sub_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            break;
        }
        case IR_OP_TYPE_MUL:
        {
            TRANSLATION_ERROR_HANDLE(write_imul_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            break;
        }
        case IR_OP_TYPE_DIV:
        {
            TRANSLATION_ERROR_HANDLE(write_xor_r_r(translator, REG_NUM_RDX, REG_NUM_RDX));
            TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RAX, REG_NUM_RBX));
            TRANSLATION_ERROR_HANDLE(write_idiv_r(translator, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_mov_r_r(translator, REG_NUM_RBX, REG_NUM_RAX));
            break;
        }
        case IR_OP_TYPE_EQ:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETE, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
            break;
        }
        case IR_OP_TYPE_NEQ:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETNE, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
            break;
        }
        case IR_OP_TYPE_LESS:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETL, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
            break;
        }
        case IR_OP_TYPE_LESSEQ:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETLE, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
            break;
        }
        case IR_OP_TYPE_GREAT:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETG, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
            break;
        }
        case IR_OP_TYPE_GREATEQ:
        {
            TRANSLATION_ERROR_HANDLE(write_cmp_r_r(translator, REG_NUM_RBX, REG_NUM_RCX));
            TRANSLATION_ERROR_HANDLE(write_cond_set(translator, OP_CODE_SETGE, REG_NUM_RBX)); //bl
            TRANSLATION_ERROR_HANDLE(write_movzx(translator, REG_NUM_RBX, REG_NUM_RBX)); // second reg - bl
        }

        default:
        {
            fprintf(stderr, "Invalid IR_OP_TYPE\n");
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
    }

    TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RBX));

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

    TRANSLATION_ERROR_HANDLE(add_not_handle_addr(translator, &func, translator->cur_addr + 1));
    TRANSLATION_ERROR_HANDLE(write_call_addr(translator, 0));

    TRANSLATION_ERROR_HANDLE(write_add_r_i(translator, REG_NUM_RSP, 8 * (int64_t)translator->cur_block->operand1_num));

    if (kIR_SYS_CALL_ARRAY[translator->cur_block->operand2_num].HaveRetVal)
    {
        TRANSLATION_ERROR_HANDLE(write_push_r(translator, REG_NUM_RAX)); // ret val
    }

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

    label_t func_out = {.name = "in"};

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func_out));

    uint8_t bytes[] = 
    {
        0xba, 0x20, 0x00, 0x00, 0x00,         // mov    $0x20,%edx
        0x41, 0xba, 0x0a, 0x00, 0x00, 0x00,   // mov    $0xa,%r10d
        0x48, 0x29, 0xd4,                     // sub    %rdx,%rsp
        0x48, 0x89, 0xe6,                     // mov    %rsp,%rsi
        0x48, 0x31, 0xc0,                     // xor    %rax,%rax
        0x48, 0x31, 0xff,                     // xor    %rdi,%rdi
        0x0f, 0x05,                           // syscall
        0x48, 0x85, 0xc0,                     // test   %rax,%rax
        0x7e, 0x48,                           // jle    4010c0 <in.ExitError>

        0x48, 0x89, 0xc1,                     // mov    %rax,%rcx
        0x48, 0xff, 0xc9,                     // dec    %rcx
        0x48, 0x31, 0xc0,                     // xor    %rax,%rax
        0x48, 0x31, 0xff,                     // xor    %rdi,%rdi
        0x8a, 0x1e,                           // mov    (%rsi),%bl
        0x80, 0xfb, 0x2d,                     // cmp    $0x2d,%bl
        0x75, 0x0b,                           // jne    401096 <in.CheckDigit>
        0x48, 0xff, 0xc7,                     // inc    %rdi
        0x48, 0xff, 0xc6,                     // inc    %rsi
        0x48, 0xff, 0xc9,                     // dec    %rcx
        0x74, 0x2a,                           // je     4010c0 <in.ExitError>

        // in.CheckDigit:
        0x8a, 0x1e,                           // mov    (%rsi),%bl
        0x80, 0xeb, 0x30,                     // sub    $0x30,%bl
        0x80, 0xfb, 0x09,                     // cmp    $0x9,%bl
        0x77, 0x20,                           // ja     4010c0 <in.ExitError>

        // in.ConvertLoop:
        0x8a, 0x1e,                           // mov    (%rsi),%bl
        0x80, 0xeb, 0x30,                     // sub    $0x30,%bl
        0x49, 0x0f, 0xaf, 0xc2,               // imul   %r10,%rax
        0x48, 0x01, 0xd8,                     // add    %rbx,%rax
        0x48, 0xff, 0xc6,                     // inc    %rsi
        0x48, 0xff, 0xc9,                     // dec    %rcx
        0x75, 0xec,                           // jne    4010a0 <in.ConvertLoop>

        0x48, 0x85, 0xff,                     // test   %rdi,%rdi
        0x74, 0x03,                           // je     4010bc <in.ExitSuccess>
        0x48, 0xf7, 0xd8,                     // neg    %rax

        // in.ExitSuccess:
        0x48, 0x01, 0xd4,                     // add    %rdx,%rsp
        0xc3,                                 // ret

        // in.ExitError:
        0x48, 0x01, 0xd4,                     // add    %rdx,%rsp
        0x48, 0x31, 0xc0,                     // xor    %rax,%rax
        0xc3                                  // ret
    };

    const size_t bytes_size = sizeof(bytes);

    TRANSLATION_ERROR_HANDLE(write_arr_text(translator, bytes, bytes_size));

    translator->cur_addr += bytes_size;

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_out_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func_out = {.name = "out"};

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func_out));

    uint8_t bytes[] = 
    {
        0x48, 0x8b, 0x44, 0x24, 0x08,      // mov    0x8(%rsp),%rax
        0x41, 0xbb, 0x0a, 0x00, 0x00, 0x00, // mov    $0xa,%r11d
        0x48, 0x89, 0xc7,                  // mov    %rax,%rdi
        0x48, 0x31, 0xc9,                  // xor    %rcx,%rcx
        0x48, 0xff, 0xcc,                  // dec    %rsp
        0xc6, 0x04, 0x24, 0x0a,            // movb   $0xa,(%rsp)
        0x48, 0xff, 0xc1,                  // inc    %rcx
        0x48, 0x85, 0xc0,                  // test   %rax,%rax
        0x78, 0x0e,                        // js     out.Negative
        0x75, 0x0f,                        // jne    out.Convertion
        0x48, 0xff, 0xcc,                  // dec    %rsp
        0xc6, 0x04, 0x24, 0x30,            // movb   $0x30,(%rsp)
        0x48, 0xff, 0xc1,                  // inc    %rcx
        0xeb, 0x36,                        // jmp    out.Print
    
        // out.Negative:
        0x48, 0xf7, 0xd8,                  // neg    %rax
    
        // out.Convertion:
        0x48, 0x31, 0xd2,                  // xor    %rdx,%rdx
        0x49, 0xf7, 0xf3,                  // div    %r11
        0x48, 0x83, 0xfa, 0x0a,            // cmp    $0xa,%rdx
        0x72, 0x06,                        // jb     out.below_10
        0x48, 0x83, 0xc2, 0x37,            // add    $0x37,%rdx
        0xeb, 0x04,                        // jmp    out.push_char
    
        // out.below_10:
        0x48, 0x83, 0xc2, 0x30,            // add    $0x30,%rdx
    
        // out.push_char:
        0x48, 0xff, 0xcc,                  // dec    %rsp
        0x88, 0x14, 0x24,                  // mov    %dl,(%rsp)
        0x48, 0xff, 0xc1,                  // inc    %rcx
        0x48, 0x85, 0xc0,                  // test   %rax,%rax
        0x75, 0xdc,                        // jne    out.Convertion
        0x48, 0x85, 0xff,                  // test   %rdi,%rdi
        0x79, 0x0a,                        // jns    out.Print
        0x48, 0xff, 0xcc,                  // dec    %rsp
        0xc6, 0x04, 0x24, 0x2d,            // movb   $0x2d,(%rsp)
        0x48, 0xff, 0xc1,                  // inc    %rcx
    
        // out.Print:
        0x48, 0x89, 0xca,                  // mov    %rcx,%rdx
        0x48, 0x89, 0xe6,                  // mov    %rsp,%rsi
        0xbf, 0x01, 0x00, 0x00, 0x00,      // mov    $0x1,%edi
        0xb8, 0x01, 0x00, 0x00, 0x00,      // mov    $0x1,%eax
        0x0f, 0x05,                        // syscall
        0x48, 0x01, 0xd4,                  // add    %rdx,%rsp
        0x48, 0x85, 0xc0,                  // test   %rax,%rax
        0x74, 0x03,                        // je     out.Exit
    
        // out.ExitSuccess:
        0x48, 0x31, 0xc0,                  // xor    %rax,%rax
    
        // out.Exit:
        0xc3                               // ret
    };

    const size_t bytes_size = sizeof(bytes);

    TRANSLATION_ERROR_HANDLE(write_arr_text(translator, bytes, bytes_size));

    translator->cur_addr += bytes_size;

    return TRANSLATION_ERROR_SUCCESS;
}

static enum TranslationError translate_syscall_pow_(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    label_t func_out = {.name = "pow"};

    TRANSLATION_ERROR_HANDLE(add_label(translator, &func_out));

    uint8_t bytes[] = 
    {
        // load arguments
        0x48, 0x8b, 0x4c, 0x24, 0x08,     // mov    0x8(%rsp),%rcx   
        0x48, 0x8b, 0x44, 0x24, 0x10,     // mov    0x10(%rsp),%rax  
        0xba, 0x01, 0x00, 0x00, 0x00,     // mov    $0x1,%edx       
    
        // Special case checks
        0x48, 0x85, 0xc9,                 // test   %rcx,%rcx 
        0x74, 0x21,                       // je     pow.done         
        
        0x48, 0x83, 0xf8, 0x01,          // cmp    $0x1,%rax  
        0x74, 0x1b,                       // je     pow.done     
        
        0x48, 0x85, 0xc0,                 // test   %rax,%rax     
        0x74, 0x1a,                       // je     pow.zero_case 
    
        // pow.pow_loop
        0x48, 0xf7, 0xc1, 0x01, 0x00, 0x00, 0x00, // test $0x1,%rcx 
        0x74, 0x04,                       // je     pow.even_power 
        
        // Odd case
        0x48, 0x0f, 0xaf, 0xd0,          // imul   %rax,%rdx    
    
        // pow.even_power
        0x48, 0x0f, 0xaf, 0xc0,           // imul   %rax,%rax     
        0x48, 0xd1, 0xe9,                 // shr    $1,%rcx         
        0x75, 0xea,                       // jne    pow.pow_loop    
    
        // pow.done
        0x48, 0x89, 0xd0,                 // mov    %rdx,%rax    
        0xc3,                             // ret
    
        // pow.zero_case
        0x48, 0x31, 0xc0,                 // xor    %rax,%rax    
        0xc3                              // ret
    };

    const size_t bytes_size = sizeof(bytes);

    TRANSLATION_ERROR_HANDLE(write_arr_text(translator, bytes, bytes_size));

    translator->cur_addr += bytes_size;

    return TRANSLATION_ERROR_SUCCESS;
}