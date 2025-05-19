#include <stdbool.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "translation/funcs/funcs.h"
#include "hash_table/libhash_table.h"
#define IR_file out
#define NUM_SPECIFER_ "%ld"
#include "PYAM_IR/include/libpyam_ir.h"
#include "translation/structs.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        const enum StackError stack_error_handler = call_func;                                      \
        if (stack_error_handler)                                                                    \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Stack error: %s\n",                               \
                            stack_strerror(stack_error_handler));                                   \
            __VA_ARGS__                                                                             \
            return IR_TRANSLATION_ERROR_STACK;                                                      \
        }                                                                                           \
    } while(0)

#define SMASH_MAP_ERROR_HANDLE_(call_func, ...)                                                     \
    do {                                                                                            \
        const enum SmashMapError error_handler = call_func;                                         \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". SmashMap error: %s\n",                            \
                            smash_map_strerror(error_handler));                                     \
            __VA_ARGS__                                                                             \
            return IR_TRANSLATION_ERROR_SMASH_MAP;                                                  \
        }                                                                                           \
    } while(0)

#define HASH_KEY_ 31
static size_t func_hash_func_(const void* const string)
{
    lassert(!is_invalid_ptr(string), "");

    size_t hash_result = 0;
 
    for (const char* it = (const char*)string; *it; ++it)
    {
        hash_result = (size_t)(((HASH_KEY_ * hash_result) % INT64_MAX + (size_t)*it) % INT64_MAX);
    }
 
    return hash_result;
}
#undef HASH_KEY_

static int map_key_to_str_ (const void* const elem, const size_t   elem_size,
                            char* const *     str,  const size_t mx_str_size)
{
    if (is_invalid_ptr(str))  return -1;
    if (is_invalid_ptr(*str)) return -1;
    (void)elem_size;

    if (elem && *(const char*)elem)
    {
        if (snprintf(*str, mx_str_size, "'%zu, %zu'", 
                                        ((const func_t*)elem)->num, 
                                        ((const func_t*)elem)->count_args) 
            <= 0)
        {
            perror("Can't snprintf key to str");
            return -1;
        }
    }
    else
    {
        if (snprintf(*str, mx_str_size, "(nul)") < 0)
        {
            perror("Can't snprintf key (nul) to str");
            return -1;
        }
    }

    return 0;
}

static int map_val_to_str_ (const void* const elem, const size_t   elem_size,
                            char* const *     str,  const size_t mx_str_size)
{
    if (is_invalid_ptr(str))  return -1;
    if (is_invalid_ptr(*str)) return -1;
    (void)elem_size;

    if (elem)
    {
        if (snprintf(*str, mx_str_size, "'%zu'", *(const size_t*)elem) <= 0)
        {
            perror("Can't snprintf val to str");
            return -1;
        }
    
    }
    else
    {
        if (snprintf(*str, mx_str_size, "(nul)") < 0)
        {
            perror("Can't snprintf key (nul) to str");
            return -1;
        }
    }

    return 0;
}


#define SMASH_MAP_SIZE_ 101
static enum IrTranslationError translator_ctor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    SMASH_MAP_ERROR_HANDLE_(
        SMASH_MAP_CTOR(
            &translator->func_arg_num, 
            SMASH_MAP_SIZE_, 
            sizeof(func_t), 
            sizeof(size_t), 
            func_hash_func_,
            map_key_to_str_,
            map_val_to_str_
        )
    );

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(stack_key_t), 1));
    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->funcs, sizeof(func_t), 10));
    translator->label_num = 0;
    translator->temp_var_num = 0;
    translator->var_num_base = 0;

    return IR_TRANSLATION_ERROR_SUCCESS;
}
#undef SMASH_MAP_SIZE_

static enum IrTranslationError clean_vars_stacks_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    while (!stack_is_empty(translator->vars))
    {
        stack_key_t poped_stack = 0;
        STACK_ERROR_HANDLE_(stack_pop(&translator->vars, &poped_stack));
        stack_dtor(&poped_stack);
    }
    translator->var_num_base = 0;

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static void translator_dtor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    smash_map_dtor(&translator->func_arg_num);

    clean_vars_stacks_(translator);

    stack_dtor(&translator->vars);
    stack_dtor(&translator->funcs);
    IF_DEBUG(translator->label_num = 0;)
    IF_DEBUG(translator->temp_var_num = 0;)
    IF_DEBUG(translator->var_num_base = 0;)
}


#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        static enum IrTranslationError translate_##name_(translator_t* const translator,            \
                                                         const tree_elem_t* elem, FILE* out);

#include "utils/src/operations/codegen.h"

#undef OPERATION_HANDLE

static enum IrTranslationError translate_recursive_(translator_t* const translator, 
                                                    const tree_elem_t* elem, 
                                                    FILE* out);


enum IrTranslationError translate(const tree_t* const tree, FILE* out)
{
    TREE_VERIFY_ASSERT(tree);
    lassert(!is_invalid_ptr(out), "");

    translator_t translator = {};
    IR_TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));

    const size_t ret_main_tmp = translator.temp_var_num++;
    
    IR_GLOBAL_VARS_NUM_(0ul); //hard cock
    IR_CALL_MAIN_(ret_main_tmp);

    IR_GIVE_ARG_(0, ret_main_tmp);
    IR_SYSCALL_(translator.temp_var_num++, 
        kIR_SYS_CALL_ARRAY[SYSCALL_HLT_INDEX].Name, 
        kIR_SYS_CALL_ARRAY[SYSCALL_HLT_INDEX].NumberOfArguments
    );

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(&translator, tree->Groot, out),     
                                translator_dtor_(&translator);
    );

    translator_dtor_(&translator);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

#define CUR_VAR_STACK_                                                                              \
        (stack_size(translator->vars)                                                               \
            ? (stack_key_t*)stack_get(translator->vars, stack_size(translator->vars) - 1)           \
            : NULL)

#define CUR_VAR_STACK_SIZE_                                                                         \
        (CUR_VAR_STACK_ ? stack_size(*CUR_VAR_STACK_) : 0ul)

#define CHECK_DECLD_VAR_(result_, elem_)                                                            \
    do {                                                                                            \
        size_t* founded_elem = NULL;                                                                \
        long long int base_counter = translator->var_num_base + (long long int)CUR_VAR_STACK_SIZE_;                   \
        for (size_t stack_ind = stack_size(translator->vars); stack_ind > 0; --stack_ind)           \
        {                                                                                           \
            const stack_key_t stack = *(stack_key_t*)stack_get(translator->vars, stack_ind - 1);    \
            base_counter -= (long long int)stack_size(stack);                                                \
            if ((founded_elem = (size_t*)stack_find(stack, &elem_->lexem.data.var, NULL)))          \
            {                                                                                       \
                result_ = base_counter                                                              \
                        + (founded_elem - (size_t*)stack_begin(stack));                             \
                break;                                                                              \
            }                                                                                       \
        }                                                                                           \
        if (!founded_elem)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Use undeclarated var with %zu num\n", elem_->lexem.data.var);          \
            return IR_TRANSLATION_ERROR_UNDECL_VAR;                                                 \
        }                                                                                           \
    } while(0)

#define CHECK_UNDECLD_VAR_(elem_)                                                                   \
    do {                                                                                            \
        if (stack_find(*CUR_VAR_STACK_, &elem_->lexem.data.var, NULL))                              \
        {                                                                                           \
            fprintf(stderr, "Redeclarated var with %zu num\n", elem_->lexem.data.var);              \
            return IR_TRANSLATION_ERROR_REDECL_VAR;                                                 \
        }                                                                                           \
    } while(0)

#define CHECK_DECLD_FUNC_(func_)                                                                    \
    do {                                                                                            \
        if (!(func_t*)stack_find(translator->funcs, &func_, NULL))                                  \
        {                                                                                           \
            fprintf(stderr, "Use undeclarated func with %zu num\n", func_.num);                     \
            return IR_TRANSLATION_ERROR_UNDECL_VAR;                                                 \
        }                                                                                           \
    } while(0)

#define CHECK_UNDECLD_FUNC_(func_)                                                                  \
    do {                                                                                            \
        if (stack_find(translator->vars, &func_, NULL))                                             \
        {                                                                                           \
            fprintf(stderr, "Redeclarated func with %zu num\n", func_.num);                         \
            return IR_TRANSLATION_ERROR_REDECL_VAR;                                                 \
        }                                                                                           \
    } while(0)


    
#define USE_LABEL_()                                                                                \
        (translator->label_num++)

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case num_: IR_TRANSLATION_ERROR_HANDLE(translate_##name_(translator, elem, out)); break;

enum IrTranslationError translate_recursive_(translator_t* const translator, const tree_elem_t* elem, 
                                             FILE* out)
{
    if (!elem) return IR_TRANSLATION_ERROR_SUCCESS;

    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    switch (elem->lexem.type)
    {
    case LEXEM_TYPE_NUM:
    {
        IR_ASSIGN_TMP_NUM_(translator->temp_var_num++, elem->lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        long long int var_ind = 0;
        CHECK_DECLD_VAR_(var_ind, elem);

        IR_ASSIGN_TMP_VAR_(translator->temp_var_num++, var_ind, "");
        break;
    }

    case LEXEM_TYPE_OP:
    {
        switch (elem->lexem.data.op)
        {

#include "utils/src/operations/codegen.h"
        
        case OP_TYPE_UNKNOWN:
        default:
            return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
        break;
    }

    case LEXEM_TYPE_END:
    default:
        return IR_TRANSLATION_ERROR_INVALID_LEXEM_TYPE;
    }

    return IR_TRANSLATION_ERROR_SUCCESS;
}
#undef OPERATION_HANDLE

static enum IrTranslationError translate_SUM(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_SUM, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_SUB(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_SUB, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_MUL, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_DIV, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {
        .num = SIZE_MAX - SYSCALL_POW_INDEX, 
        .count_args = (size_t)kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].NumberOfArguments
    };

    if (!smash_map_get_val(&translator->func_arg_num, &func))
    {
        size_t temp = 0;
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->func_arg_num, 
                (smash_map_elem_t){.key = &func, .val = &temp}
            )
        );
    }

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_GIVE_ARG_(0, first_op);
    IR_GIVE_ARG_(1, second_op);

    IR_SYSCALL_(translator->temp_var_num++, kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].Name, 
        kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].NumberOfArguments
    );

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, second_op, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    CHECK_UNDECLD_VAR_(elem->lt);
    STACK_ERROR_HANDLE_(stack_push(CUR_VAR_STACK_, &elem->lt->lexem.data.var));

    const long long int first_op = translator->var_num_base + (long long int)(CUR_VAR_STACK_SIZE_ - 1);

    if (elem->rt)
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    else
    {
        IR_ASSIGN_TMP_NUM_(translator->temp_var_num++, 0l);
    }

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, second_op, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError create_new_var_frame_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    translator->var_num_base += (long long int)CUR_VAR_STACK_SIZE_;
    stack_key_t stack_temp = 0;
    STACK_ERROR_HANDLE_(STACK_CTOR(&stack_temp, sizeof(size_t), 10));
    STACK_ERROR_HANDLE_(stack_push(&translator->vars, &stack_temp));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError delete_top_var_frame_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    stack_dtor(CUR_VAR_STACK_);
    STACK_ERROR_HANDLE_(stack_pop(&translator->vars, NULL));
    translator->var_num_base -= (long long int)CUR_VAR_STACK_SIZE_;

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t cond_res = translator->temp_var_num - 1;

    size_t label_else = USE_LABEL_();
    size_t label_help = USE_LABEL_();

    IR_COND_JMP_(label_help, cond_res, "check IF condition, jmp after else");
    IR_JMP_(label_else, "jmp to else in IF");
    IR_LABEL_(label_help, "label not else in IF");

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    if (elem->rt->rt)
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        size_t label_end = USE_LABEL_();

        IR_JMP_(label_end, "jmp to end IF");
        IR_LABEL_(label_else, "label else for IF");

        STACK_ERROR_HANDLE_(stack_clean(CUR_VAR_STACK_));

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));

        IR_LABEL_(label_end, "label end for IF");
    }
    else
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        IR_LABEL_(label_else, "label else for IF");
    }

    IR_TRANSLATION_ERROR_HANDLE(delete_top_var_frame_(translator));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    size_t label_condition  = 0;
    size_t label_body       = 0;
    size_t label_else       = 0;
    size_t label_end        = 0;

    if (elem->rt->rt)
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

        const size_t cond_res = translator->temp_var_num - 1;

        label_body = USE_LABEL_();

        IR_COND_JMP_(label_body, cond_res, "first check WHILE condition, jump to body WHILE");

        label_else = USE_LABEL_();

        IR_JMP_(label_else, "jmp to else WHILE");
    }

    label_condition = USE_LABEL_();
    IR_LABEL_(label_condition, "start condition WHILE");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t cond_res = translator->temp_var_num - 1;

    label_end = USE_LABEL_();

    IR_COND_JMP_(label_body, cond_res, "second check WHILE condition, jmp to body");

    IR_JMP_(label_end,  "jmp to end");

    IR_LABEL_(label_body, "start body WHILE");

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

    IR_JMP_(label_condition, "jump to condition WHILE");

    if (elem->rt->rt)
    {
        STACK_ERROR_HANDLE_(stack_clean(CUR_VAR_STACK_));

        IR_LABEL_(label_else, "start else WHILE");

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
    }

    IR_LABEL_(label_end, "end WHILE");

    IR_TRANSLATION_ERROR_HANDLE(delete_top_var_frame_(translator));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    func_t func = {
        .num = SIZE_MAX - SYSCALL_POW_INDEX, 
        .count_args = (size_t)kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].NumberOfArguments
    };

    if (!smash_map_get_val(&translator->func_arg_num, &func))
    {
        size_t temp = 0;
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->func_arg_num, 
                (smash_map_elem_t){.key = &func, .val = &temp}
            )
        );
    }

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    const size_t first_op_tmp = translator->temp_var_num++;
    const size_t op_res_tmp = translator->temp_var_num++;

    IR_ASSIGN_TMP_VAR_(first_op_tmp, first_op, "");

    IR_GIVE_ARG_(0, first_op_tmp);
    IR_GIVE_ARG_(1, second_op);

    IR_SYSCALL_(op_res_tmp, kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].Name, 
        kIR_SYS_CALL_ARRAY[SYSCALL_POW_INDEX].NumberOfArguments
    );
    IR_ASSIGN_VAR_(first_op, op_res_tmp, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    const size_t first_op_tmp = translator->temp_var_num++;
    const size_t op_res_tmp = translator->temp_var_num++;

    IR_ASSIGN_TMP_VAR_(first_op_tmp, first_op, "");
    IR_OPERATION_(op_res_tmp, IR_OP_TYPE_SUM, first_op_tmp, second_op);
    IR_ASSIGN_VAR_(first_op, op_res_tmp, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    const size_t first_op_tmp = translator->temp_var_num++;
    const size_t op_res_tmp = translator->temp_var_num++;

    IR_ASSIGN_TMP_VAR_(first_op_tmp, first_op, "");
    IR_OPERATION_(op_res_tmp, IR_OP_TYPE_SUB, first_op_tmp, second_op);
    IR_ASSIGN_VAR_(first_op, op_res_tmp, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    const size_t first_op_tmp = translator->temp_var_num++;
    const size_t op_res_tmp = translator->temp_var_num++;

    IR_ASSIGN_TMP_VAR_(first_op_tmp, first_op, "");
    IR_OPERATION_(op_res_tmp, IR_OP_TYPE_MUL, first_op_tmp, second_op);
    IR_ASSIGN_VAR_(first_op, op_res_tmp, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long long int first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    const size_t first_op_tmp = translator->temp_var_num++;
    const size_t op_res_tmp = translator->temp_var_num++;

    IR_ASSIGN_TMP_VAR_(first_op_tmp, first_op, "");
    IR_OPERATION_(op_res_tmp, IR_OP_TYPE_DIV, first_op_tmp, second_op);
    IR_ASSIGN_VAR_(first_op, op_res_tmp, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_EQ, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_NEQ, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_LESS, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_LESSEQ, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_GREAT, first_op, second_op);


    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_OPERATION_(translator->temp_var_num++, IR_OP_TYPE_GREATEQ, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError init_func_(translator_t* const translator, const tree_elem_t* tree_ptr,
                                          func_t* const func)
{
    lassert(!is_invalid_ptr(func), "");
    lassert(!is_invalid_ptr(tree_ptr), "");
    lassert(!is_invalid_ptr(translator), "");

    func->num        = tree_ptr->lt->lexem.data.var;
    func->count_args = (tree_ptr->rt != NULL);

    for (tree_ptr = tree_ptr->rt; tree_ptr != NULL; tree_ptr = tree_ptr->lt)
    {
        func->count_args += (tree_ptr->lexem.type    == LEXEM_TYPE_OP 
                          && tree_ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    if (!smash_map_get_val(&translator->func_arg_num, func))
    {
        size_t temp = 0;
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->func_arg_num, 
                (smash_map_elem_t){.key = func, .val = &temp}
            )
        );
    }

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_FUNC(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {};
    IR_TRANSLATION_ERROR_HANDLE(init_func_(translator, elem->lt, &func));

    CHECK_UNDECLD_FUNC_(func);
    STACK_ERROR_HANDLE_(stack_push(&translator->funcs, &func));

    IR_FUNCTION_BODY_(func.num, func.count_args, (size_t)elem->lt->lexem.data.num, ""); 

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    const tree_elem_t** arr_vars = calloc(func.count_args, sizeof(*arr_vars));

    const tree_elem_t* arg = elem->lt->rt;
    for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    {
        arr_vars[count - 1] = arg->rt;
    }
    if (func.count_args != 0)
    {
        arr_vars[func.count_args - 1] = arg;
    }

    for (size_t var_ind = 0; var_ind < func.count_args; ++var_ind)
    {
        arg = arr_vars[func.count_args - var_ind - 1];
        CHECK_UNDECLD_VAR_(arg);
        STACK_ERROR_HANDLE_(stack_push(CUR_VAR_STACK_, &arg->lexem.data.var));

        const long long int first_op = translator->var_num_base + (long long int)(CUR_VAR_STACK_SIZE_ - 1);
        const size_t second_op = var_ind;

        IR_TAKE_ARG_(first_op, second_op, "");
    }

    free(arr_vars);


    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    IR_TRANSLATION_ERROR_HANDLE(clean_vars_stacks_(translator));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {};
    IR_TRANSLATION_ERROR_HANDLE(init_func_(translator, elem, &func));

    // const tree_elem_t* arg = elem->rt;
    // for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    // {
    //     const size_t first_op = start_arg_num + func.count_args - count;

    //     IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg->rt, out));
    //     const size_t second_op = translator->temp_var_num - 1;

    //     IR_GIVE_ARG_(first_op, second_op);
    // }
    // if (func.count_args != 0)
    // {
    //     const size_t first_op = start_arg_num;

    //     IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg, out));
    //     const size_t second_op = translator->temp_var_num - 1;

    //     IR_GIVE_ARG_(first_op, second_op);
    // }

    const tree_elem_t** arr_vars = calloc(func.count_args, sizeof(*arr_vars));

    const tree_elem_t* arg = elem->rt;
    for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    {
        arr_vars[count - 1] = arg->rt;
    }
    if (func.count_args != 0)
    {
        arr_vars[func.count_args - 1] = arg;
    }

    for (size_t var_ind = 0; var_ind < func.count_args; ++var_ind)
    {
        const size_t first_op = var_ind;

        arg = arr_vars[func.count_args - var_ind - 1];
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg, out));
        const size_t second_op = translator->temp_var_num - 1;

        IR_GIVE_ARG_(first_op, second_op);
    }

    free(arr_vars);

    IR_CALL_FUNC_(translator->temp_var_num++, func.num, func.count_args, "");

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_MAIN(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_MAIN_BODY_((size_t)elem->lt->lexem.data.num);

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt->lt, out));
    IR_TRANSLATION_ERROR_HANDLE(clean_vars_stacks_(translator));

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));


    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_ARGS_COMMA(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    // IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    // IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_CALL_FUNC_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_CALL_FUNC_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    (void)translator;
    (void)elem;
    (void)out;

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return IR_TRANSLATION_ERROR_INVALID_OP_TYPE;
}

static enum IrTranslationError translate_RET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t ret_val = translator->temp_var_num - 1;

    IR_RET_(ret_val);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_IN(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    size_t count_args = (elem->lt != NULL);

    for (const tree_elem_t* tree_ptr = elem->lt; tree_ptr != NULL; tree_ptr = tree_ptr->lt)
    {
        count_args += (tree_ptr->lexem.type    == LEXEM_TYPE_OP 
                    && tree_ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    long long int* arr_vars = calloc(count_args, sizeof(*arr_vars));

    const tree_elem_t* arg = elem->lt;
    for (size_t count = 1; count < count_args; ++count, arg = arg->lt)
    {
        long long int var_in = 0;
        CHECK_DECLD_VAR_(var_in, arg->rt);
        arr_vars[count - 1] = var_in;
    }
    if (count_args != 0)
    {
        long long int var_in = 0;
        CHECK_DECLD_VAR_(var_in, arg);
        arr_vars[count_args - 1] = var_in;
    }

    for (size_t var_ind = 0; var_ind < count_args; ++var_ind)
    {
        long long int var = arr_vars[count_args - var_ind - 1];
        const size_t tmp_num = translator->temp_var_num++;

        IR_SYSCALL_(
            tmp_num, 
            kIR_SYS_CALL_ARRAY[SYSCALL_IN_INDEX].Name, 
            kIR_SYS_CALL_ARRAY[SYSCALL_IN_INDEX].NumberOfArguments
        );

        IR_ASSIGN_VAR_(var, tmp_num, "IN arg");
    }

    free(arr_vars);

    return IR_TRANSLATION_ERROR_SUCCESS;
}


static enum IrTranslationError translate_OUT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {
        .num = SIZE_MAX - SYSCALL_OUT_INDEX, 
        .count_args = (size_t)kIR_SYS_CALL_ARRAY[SYSCALL_OUT_INDEX].NumberOfArguments
    };

    if (!smash_map_get_val(&translator->func_arg_num, &func))
    {
        size_t temp = 0;
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->func_arg_num, 
                (smash_map_elem_t){.key = &func, .val = &temp}
            )
        );
    }

    size_t count_args = (elem->lt != NULL);

    for (const tree_elem_t* tree_ptr = elem->lt; tree_ptr != NULL; tree_ptr = tree_ptr->lt)
    {
        count_args += (tree_ptr->lexem.type    == LEXEM_TYPE_OP 
                    && tree_ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    const tree_elem_t** arr_vars = calloc(count_args, sizeof(*arr_vars));

    const tree_elem_t* arg = elem->lt;
    for (size_t count = 1; count < count_args; ++count, arg = arg->lt)
    {
        arr_vars[count - 1] = arg->rt;
    }
    if (count_args != 0)
    {
        arr_vars[count_args - 1] = arg;
    }

    for (size_t var_ind = 0; var_ind < count_args; ++var_ind)
    {
        arg = arr_vars[count_args - var_ind - 1];
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg, out));
        size_t argument = translator->temp_var_num - 1;

        IR_GIVE_ARG_(0, argument);

        IR_SYSCALL_(
            translator->temp_var_num++, 
            kIR_SYS_CALL_ARRAY[SYSCALL_OUT_INDEX].Name, 
            kIR_SYS_CALL_ARRAY[SYSCALL_OUT_INDEX].NumberOfArguments
        );
    }

    free(arr_vars);

    return IR_TRANSLATION_ERROR_SUCCESS;
}
