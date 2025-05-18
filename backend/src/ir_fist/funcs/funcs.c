#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "funcs.h"
#include "utils/utils.h"
#include "ir_fist/verification/verification.h"
#include "ir_fist/structs.h"

#define FIST_ERROR_HANDLE_(call_func, ...)                                                          \
    do {                                                                                            \
        enum FistError error_handler = call_func;                                                   \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            fist_strerror(error_handler));                                          \
            __VA_ARGS__                                                                             \
            return IR_FIST_ERROR_FIST;                                                              \
        }                                                                                           \
    } while(0)

enum IrFistError ir_block_init(ir_block_t* const block)
{
    lassert(!is_invalid_ptr(block), "");
    
    block->type = IR_OP_BLOCK_TYPE_INVALID;

    block->label_type = IR_OPERAND_TYPE_NONE;
    block->operand1_type = IR_OPERAND_TYPE_NONE;
    block->operand2_type = IR_OPERAND_TYPE_NONE;
    block->operation_type = IR_OPERAND_TYPE_NONE;
    block->ret_type = IR_OPERAND_TYPE_NONE;

    block->operand1_num = 0;
    block->operand2_num = 0;
    block->operation_num = IR_OP_TYPE_INVALID_OPERATION;
    block->ret_num = 0;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_CALL_FUNCTION(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_FUNCTION_BODY(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_COND_JUMP(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_ASSIGNMENT(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_OPERATION(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_RETURN(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_LABEL(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_SYSCALL(ir_block_t* const block, const char** const cur_text_pos);
static enum IrFistError parse_GLOBAL_VARS(ir_block_t* const block, const char** const cur_text_pos);

static int str_from_file_(const char* const filename, char** str, size_t* const str_size);

#define IR_OP_BLOCK_HANDLE(num_, name_)                                                             \
    parse_##name_,

enum IrFistError ir_fist_ctor(fist_t* fist, const char* const filename)
{
    lassert(!is_invalid_ptr(fist), "");
    lassert(!is_invalid_ptr(filename), "");

    char* text = NULL;
    size_t text_size = 0;
    if (str_from_file_(filename, &text, &text_size))
    {
        fist_dtor(fist);
        fprintf(stderr, "Can't str_from_file_\n");
        return IR_FIST_ERROR_STANDARD_ERRNO;
    }

    enum IrFistError (*ir_blocks[])(ir_block_t* const block, const char** const cur_text_pos) = {

#include "PYAM_IR/include/codegen.h"

    };

    const size_t ir_blocks_size = sizeof(ir_blocks) / sizeof(*ir_blocks);

    const char* cur_text_pos = text;
    size_t handled_block_cnt = 0;


    while (isspace(*cur_text_pos))
    {
        ++cur_text_pos;
    }

    while (*cur_text_pos != '\0')
    {
        ir_block_t block = {};
        IR_FIST_ERROR_HANDLE(
            ir_block_init(&block),     
            fist_dtor(fist);
            munmap(text, text_size);
        );

        bool is_parsed = false;
        // fprintf(stderr, RED_TEXT("NEXT:\n") "'%s'", cur_text_pos);

        for (size_t ir_block_ind = 0; ir_block_ind < ir_blocks_size; ++ir_block_ind)
        {
            if (ir_blocks[ir_block_ind](&block, &cur_text_pos) == IR_FIST_ERROR_SUCCESS)
            {
                // fprintf(stderr, "it: %zu, ind: %zu\n",  handled_block_cnt, ir_block_ind);
                is_parsed = true;
                break;
            }
        }
        
        if (!is_parsed)
        {
            fist_dtor(fist);
            munmap(text, text_size);
            fprintf(stderr, "Can't parse some block\n");
            return IR_FIST_ERROR_PARSE_BLOCK;
        }

        FIST_ERROR_HANDLE_(
            fist_push(fist, handled_block_cnt++, &block),
            fist_dtor(fist);
            munmap(text, text_size);
        );

        while (isspace(*cur_text_pos))
        {
            ++cur_text_pos;
        }
    }

    munmap(text, text_size);

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_CALL_FUNCTION(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "call func func\n");

    block->type = IR_OP_BLOCK_TYPE_CALL_FUNCTION;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "RingRing(tmp%zu, %128[^)]%*[^\n]%n", 
            &block->ret_num, block->label_str, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_TMP;
        block->label_type = IR_OPERAND_TYPE_LABEL;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_FUNCTION_BODY(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "func body func\n");

    block->type = IR_OP_BLOCK_TYPE_FUNCTION_BODY;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Gyat(%128[^,], %zu, %zu%*[^\n]%n", 
            block->label_str, &block->operand1_num, &block->operand2_num, &read_sym_cnt) 
        >= 3)
    {
        block->label_type = IR_OPERAND_TYPE_LABEL;
        block->operand1_type = IR_OPERAND_TYPE_NUM;
        block->operand2_type = IR_OPERAND_TYPE_NUM;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;

}

static enum IrFistError parse_COND_JUMP(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "jmp func\n");

    block->type = IR_OP_BLOCK_TYPE_COND_JUMP;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Frog(%128[^,], tmp%zu%*[^\n]%n", 
            block->label_str, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->label_type = IR_OPERAND_TYPE_LABEL;
        block->operand1_type = IR_OPERAND_TYPE_TMP;
    }
    else 
    if (sscanf(*cur_text_pos, "Frog(%128[^,], %zu%*[^\n]%n", 
            block->label_str, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->label_type = IR_OPERAND_TYPE_LABEL;
        block->operand1_type = IR_OPERAND_TYPE_NUM;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_ASSIGNMENT(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "assign func\n");

    block->type = IR_OP_BLOCK_TYPE_ASSIGNMENT;

    int read_sym_cnt = 0;


    if (sscanf(*cur_text_pos, "Gnoming(tmp%zu, var%zu%*[^\n]%n", 
            &block->ret_num, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_TMP;
        block->operand1_type = IR_OPERAND_TYPE_VAR;
    }
    else 
    if (sscanf(*cur_text_pos, "Gnoming(tmp%zu, %zu%*[^\n]%n", 
            &block->ret_num, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_TMP;
        block->operand1_type = IR_OPERAND_TYPE_NUM;
    }
    else 
    if (sscanf(*cur_text_pos, "Gnoming(var%zu, tmp%zu%*[^\n]%n", 
            &block->ret_num, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_VAR;
        block->operand1_type = IR_OPERAND_TYPE_TMP;
    }
    else 
    if (sscanf(*cur_text_pos, "Gnoming(var%zu, arg%zu%*[^\n]%n", 
            &block->ret_num, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_VAR;
        block->operand1_type = IR_OPERAND_TYPE_ARG;
    }
    else 
    if (sscanf(*cur_text_pos, "Gnoming(arg%zu, tmp%zu%*[^\n]%n", 
            &block->ret_num, &block->operand1_num, &read_sym_cnt) 
        >= 2)
    {
        block->ret_type = IR_OPERAND_TYPE_ARG;
        block->operand1_type = IR_OPERAND_TYPE_TMP;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;

}

static enum IrFistError parse_OPERATION(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "op func\n");

    block->type = IR_OP_BLOCK_TYPE_OPERATION;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Digging(tmp%zu, %d, tmp%zu, tmp%zu%*[^\n]%n", 
            &block->ret_num, &block->operation_num, &block->operand1_num, &block->operand2_num, 
            &read_sym_cnt) 
        >= 4)
    {
        block->ret_type = IR_OPERAND_TYPE_TMP;
        block->operation_type = IR_OPERAND_TYPE_OPERATION;
        block->operand1_type = IR_OPERAND_TYPE_TMP;
        block->operand2_type = IR_OPERAND_TYPE_TMP;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_RETURN(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "ret func\n");

    block->type = IR_OP_BLOCK_TYPE_RETURN;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Cherepovec(tmp%zu%*[^\n]%n", 
            &block->ret_num, &read_sym_cnt) 
        >= 1)
    {
        block->ret_type = IR_OPERAND_TYPE_TMP;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_LABEL(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "label func\n");

    block->type = IR_OP_BLOCK_TYPE_LABEL;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Viperr(%128[^)]%*[^\n]%n", 
            block->label_str, &read_sym_cnt) 
        >= 1)
    {
        block->label_type = IR_OPERAND_TYPE_LABEL;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_SYSCALL(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");
    // fprintf(stderr, "syscall func\n");

    block->type = IR_OP_BLOCK_TYPE_SYSCALL;

    int read_sym_cnt = 0;

    if (sscanf(*cur_text_pos, "Bobb(tmp%zu, %128[^,], %zu%*[^\n]%n", 
            &block->ret_num, block->label_str, &block->operand1_num, &read_sym_cnt) 
        >= 3)
    {
        block->operand2_num = 0;
        for (; block->operand2_num < kIR_SYS_CALL_NUMBER; ++block->operand2_num)
        {
            if (strcmp(block->label_str, kIR_SYS_CALL_ARRAY[block->operand2_num].Name) == 0)
            {
                break;
            }
        }
        
        block->ret_num = IR_OPERAND_TYPE_TMP;
        block->label_type = IR_OPERAND_TYPE_LABEL;
        block->operand1_type = IR_OPERAND_TYPE_NUM;
        block->operand2_type = IR_OPERAND_TYPE_NUM;
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static enum IrFistError parse_GLOBAL_VARS(ir_block_t* const block, const char** const cur_text_pos)
{
    lassert(!is_invalid_ptr(block), "");
    lassert(!is_invalid_ptr(cur_text_pos), "");
    lassert(!is_invalid_ptr(*cur_text_pos), "");

    block->type = IR_OP_BLOCK_TYPE_GLOBAL_VARS;

    // fprintf(stderr, "gg func\n");

    int read_sym_cnt = 0;

    size_t tmp_num = 0;

    if (sscanf(*cur_text_pos, "Gg(%zu%*[^\n]%n", 
            &tmp_num, &read_sym_cnt) 
        >= 1)
    {
        /*nothing*/
    }
    else
    {
        return IR_FIST_ERROR_PARSE_BLOCK;
    }
    // fprintf(stderr, "gg read_sym_cnt: %zu\n", read_sym_cnt);

    *cur_text_pos += read_sym_cnt;

    return IR_FIST_ERROR_SUCCESS;
}

static int str_size_from_file_(size_t* const str_size, const int fd);

static int str_from_file_(const char* const filename, char** str, size_t* const str_size)
{
    lassert(!is_invalid_ptr(filename), "");
    lassert(!is_invalid_ptr(str), "");
    lassert(str_size, "");

    int fd = open(filename, O_RDWR);
    if (fd == -1)
    {
        perror("Can't fopen input file");
        return 1;
    }

    if (str_size_from_file_(str_size, fd))
    {
        fprintf(stderr, "Can't str_size_from_file_\n");
        close(fd);
        return 1;
    }

    *str = mmap(NULL, *str_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (*str == MAP_FAILED)
    {
        perror("Can't mmap");
        close(fd);
        return 1;
    }

    if (close(fd))
    {
        perror("Can't fclose input file");
        return 1;
    }
    IF_DEBUG(fd = -1;)

    return 0;
}

static int str_size_from_file_(size_t* const str_size, const int fd)
{
    lassert(str_size, "");
    lassert(fd != -1, "");

    struct stat fd_stat = {};

    if (fstat(fd, &fd_stat))
    {
        perror("Can't fstat");
        return 1;
    }

    *str_size = (size_t)fd_stat.st_size;

    return 0;
}