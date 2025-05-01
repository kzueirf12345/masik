#include <stdbool.h>

#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "translation/funcs/funcs.h"
#include "hash_table/libhash_table.h"
#include "PYAM_IR/libpyam_ir.h"

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

typedef struct Func
{
    size_t num;
    size_t count_args;
} func_t;

typedef struct Translator
{
    stack_key_t vars;
    stack_key_t funcs;
    size_t label_num;
    size_t temp_var_num;
    long var_num_base;
    size_t arg_var_num;

    smash_map_t func_arg_num;
} translator_t;

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
    translator->arg_var_num = 0;

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
    IF_DEBUG(translator->arg_var_num = 0;)
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

    IR_CALL_MAIN_(translator.temp_var_num++);

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
        long base_counter = translator->var_num_base + (long)CUR_VAR_STACK_SIZE_;                   \
        for (size_t stack_ind = stack_size(translator->vars); stack_ind > 0; --stack_ind)           \
        {                                                                                           \
            const stack_key_t stack = *(stack_key_t*)stack_get(translator->vars, stack_ind - 1);    \
            base_counter -= (long)stack_size(stack);                                                \
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
        IR_NUM_(translator->temp_var_num++, elem->lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        long var_ind = 0;
        CHECK_DECLD_VAR_(var_ind, elem);

        IR_VAR_(translator->temp_var_num++, var_ind);
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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_SUM, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_SUB, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_MUL, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_DIV, first_op, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t first_op = translator->temp_var_num - 1;

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_POW, first_op, second_op);

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

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_ASSIGNMENT, second_op);

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

    const long first_op = translator->var_num_base + (long)(CUR_VAR_STACK_SIZE_ - 1);

    if (elem->rt)
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    else
    {
        IR_NUM_(translator->temp_var_num++, 0l);
    }

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_DECL_ASSIGNMENT, second_op);

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

    translator->var_num_base += (long)CUR_VAR_STACK_SIZE_;
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
    translator->var_num_base -= (long)CUR_VAR_STACK_SIZE_;

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

    IR_COND_JMP_(label_else, cond_res);

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    if (elem->rt->rt)
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        size_t label_end = USE_LABEL_();

        IR_JMP_(label_end);
        IR_LABEL_(label_else);

        STACK_ERROR_HANDLE_(stack_clean(CUR_VAR_STACK_));

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));

        IR_LABEL_(label_end);
    }
    else
    {
        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        IR_LABEL_(label_else);
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
        label_else = USE_LABEL_();

        IR_COND_JMP_(label_else, cond_res);

        label_body = USE_LABEL_();

        IR_JMP_(label_body);
    }

    label_condition = USE_LABEL_();
    IR_LABEL_(label_condition);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    const size_t cond_res = translator->temp_var_num - 1;
    label_end = USE_LABEL_();

    IR_COND_JMP_(label_end, cond_res);

    IR_LABEL_(label_body);

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

    IR_JMP_(label_condition);

    if (elem->rt->rt)
    {
        STACK_ERROR_HANDLE_(stack_clean(CUR_VAR_STACK_));

        IR_LABEL_(label_else);

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
    }

    IR_LABEL_(label_end);

    IR_TRANSLATION_ERROR_HANDLE(delete_top_var_frame_(translator));

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_POW_ASSIGNMENT, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_SUM_ASSIGNMENT, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_SUB_ASSIGNMENT, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_MUL_ASSIGNMENT, second_op);

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    long first_op = 0;
    CHECK_DECLD_VAR_(first_op, elem->lt);

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    const size_t second_op = translator->temp_var_num - 1;

    IR_ASSIGN_VAR_(first_op, IR_OP_TYPE_DIV_ASSIGNMENT, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_EQ, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_NEQ, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_LESS, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_LESSEQ, first_op, second_op);

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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_GREAT, first_op, second_op);


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

    IR_ASSIGN_(translator->temp_var_num++, IR_OP_TYPE_GREATEQ, first_op, second_op);

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
                                          func_t* const func, size_t* const start_arg_num)
{
    lassert(!is_invalid_ptr(func), "");
    lassert(!is_invalid_ptr(tree_ptr), "");
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(start_arg_num), "");

    func->num        = tree_ptr->lt->lexem.data.var;
    func->count_args = (tree_ptr->rt != NULL);

    for (tree_ptr = tree_ptr->rt; tree_ptr != NULL; tree_ptr = tree_ptr->lt)
    {
        func->count_args += (tree_ptr->lexem.type    == LEXEM_TYPE_OP 
                          && tree_ptr->lexem.data.op == OP_TYPE_ARGS_COMMA);
    }

    size_t* start_arg_num_ptr = smash_map_get_val(&translator->func_arg_num, func);

    if (!start_arg_num_ptr)
    {
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->func_arg_num, 
                (smash_map_elem_t){.key = func, .val = &translator->arg_var_num}
            )
        );
        *start_arg_num = translator->arg_var_num;
        translator->arg_var_num += func->count_args;
    }
    else
    {
        *start_arg_num = *start_arg_num_ptr;
    }

    return IR_TRANSLATION_ERROR_SUCCESS;
}

static enum IrTranslationError translate_FUNC(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    func_t func = {};
    size_t start_arg_num = 0;
    IR_TRANSLATION_ERROR_HANDLE(init_func_(translator, elem->lt, &func, &start_arg_num));

    CHECK_UNDECLD_FUNC_(func);
    STACK_ERROR_HANDLE_(stack_push(&translator->funcs, &func));

    IR_IMPLEMENT_FUNC_(func.num, func.count_args);

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    const tree_elem_t* arg = elem->lt->rt;
    for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    {
        CHECK_UNDECLD_VAR_(arg->rt);
        STACK_ERROR_HANDLE_(stack_push(CUR_VAR_STACK_, &arg->rt->lexem.data.var));

        const long first_op = translator->var_num_base + (long)(CUR_VAR_STACK_SIZE_ - 1);
        const size_t second_op = start_arg_num + func.count_args - count;

        IR_TAKE_ARG_(first_op, second_op);
    }
    if (func.count_args != 0)
    {
        CHECK_UNDECLD_VAR_(arg);
        STACK_ERROR_HANDLE_(stack_push(CUR_VAR_STACK_, &arg->lexem.data.var));

        const long first_op = translator->var_num_base + (long)(CUR_VAR_STACK_SIZE_ - 1);
        const size_t second_op = start_arg_num;
        
        IR_TAKE_ARG_(first_op, second_op);
    }

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
    size_t start_arg_num = 0;
    IR_TRANSLATION_ERROR_HANDLE(init_func_(translator, elem, &func, &start_arg_num));

    const tree_elem_t* arg = elem->rt;
    for (size_t count = 1; count < func.count_args; ++count, arg = arg->lt)
    {
        const size_t first_op = start_arg_num + func.count_args - count;

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg->rt, out));
        const size_t second_op = translator->temp_var_num - 1;

        IR_GIVE_ARG_(first_op, second_op);
    }
    if (func.count_args != 0)
    {
        const size_t first_op = start_arg_num;

        IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, arg, out));
        const size_t second_op = translator->temp_var_num - 1;

        IR_GIVE_ARG_(first_op, second_op);
    }

    IR_CALL_FUNC_(translator->temp_var_num++, func.num, func.count_args);

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

    IR_IMPLEMENT_MAIN_();

    IR_TRANSLATION_ERROR_HANDLE(create_new_var_frame_(translator));

    IR_TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
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