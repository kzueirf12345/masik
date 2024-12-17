#include "translation/translation.h"
#include "utils/utils.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* translation_strerror(const enum TranslationError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_STACK);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_UNDECL_VAR);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_INVALID_OP_TYPE);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_INVALID_LEXEM_TYPE);
        CASE_ENUM_TO_STRING_(TRANSLATION_ERROR_REDECL_VAR);
        default:
            return "UNKNOWN_TRANSLATION_ERROR";
    }
    return "UNKNOWN_TRANSLATION_ERROR";
}
#undef CASE_ENUM_TO_STRING_

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

typedef struct Translator
{
    stack_key_t vars;
    size_t label_num;
} translator_t;

enum TranslationError translator_ctor_(translator_t* const translator);
void                  translator_dtor_(translator_t* const translator);

enum TranslationError translator_ctor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));
    translator->label_num = 0;

    return TRANSLATION_ERROR_SUCCESS;
}

void translator_dtor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    stack_dtor(&translator->vars);
    translator->label_num = 0;
}

enum TranslationError translate_SUM(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_SUB(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out);
enum TranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out);

enum TranslationError translate_recursive_(translator_t* const translator, const tree_elem_t* elem, 
                                           FILE* out);

enum TranslationError translate(const tree_t* const tree, FILE* out)
{
    TREE_VERIFY(tree);
    lassert(!is_invalid_ptr(out), "");

    translator_t translator = {};
    TRANSLATION_ERROR_HANDLE(translator_ctor_(&translator));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(&translator, tree->Groot, out),     
                             translator_dtor_(&translator);
    );

    translator_dtor_(&translator);

    fprintf(out, "OUT\n"); //FIXME
    fprintf(out, "HLT\n");

    return TRANSLATION_ERROR_SUCCESS;
}

#define CHECK_DECLD_VAR_(result_, elem_)                                                            \
    do {                                                                                            \
        if (!(result_ = (size_t*)stack_find(translator->vars, &elem_->lexem.data.var)))             \
        {                                                                                           \
            fprintf(stderr, "Use undeclarated var with %zu num\n", elem_->lexem.data.var);          \
            return TRANSLATION_ERROR_UNDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)

#define CHECK_UNDECLD_VAR_(elem_)                                                                   \
    do {                                                                                            \
        if (stack_find(translator->vars, &elem_->lexem.data.var))                                   \
        {                                                                                           \
            fprintf(stderr, "Redeclarated var with %zu num\n", elem_->lexem.data.var);              \
            return TRANSLATION_ERROR_REDECL_VAR;                                                    \
        }                                                                                           \
    } while(0)

#define VAR_IND_(var_)                                                                              \
        (var_ - (size_t*)stack_begin(translator->vars))
    
#define USE_LABEL_ (translator->label_num++)

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case num_: TRANSLATION_ERROR_HANDLE(translate_##name_(translator, elem, out)); break;
    

enum TranslationError translate_recursive_(translator_t* const translator, const tree_elem_t* elem, 
                                           FILE* out)
{
    if (!elem) return TRANSLATION_ERROR_SUCCESS;

    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    switch (elem->lexem.type)
    {
    case LEXEM_TYPE_NUM:
    {
        fprintf(out, "PUSH %ld\n", elem->lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        size_t* finded_var = NULL;
        CHECK_DECLD_VAR_(finded_var, elem);
        fprintf(out, "PUSH [%ld]\n", finded_var - (size_t*)stack_begin(translator->vars));
        break;
    }

    case LEXEM_TYPE_OP:
    {
        switch (elem->lexem.data.op)
        {

#include "utils/src/operations/codegen.h"
        
        case OP_TYPE_UNKNOWN:
        default:
            return TRANSLATION_ERROR_INVALID_OP_TYPE;
        }
        break;
    }

    case LEXEM_TYPE_END:
    default:
        return TRANSLATION_ERROR_INVALID_LEXEM_TYPE;
    }

    return TRANSLATION_ERROR_SUCCESS;
}
#undef OPERATION_HANDLE

enum TranslationError translate_SUM(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "ADD\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_SUB(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "SUB\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "MUL\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "DIV\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POW\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    CHECK_UNDECLD_VAR_(elem->lt);
    STACK_ERROR_HANDLE_(stack_push(&translator->vars, &elem->lt->lexem.data.var));
    size_t var_ind = stack_size(translator->vars) - 1;

    if (elem->rt)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    else
    {
        fprintf(out, "PUSH 0\n");
    }

    fprintf(out, "POP [%zu]\n", var_ind);

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));

    size_t label_else = USE_LABEL_;

    fprintf(out, "PUSH 0\n");
    fprintf(out, "JE :label%zu\n", label_else);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));

        size_t label_end = USE_LABEL_;
        fprintf(out, "JMP :label%zu\n", label_end);

        fprintf(out, ":label%zu\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
        fprintf(out, ":label%zu\n", label_end);
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
        fprintf(out, ":label%zu\n", label_else);
    }

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}

enum TranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    size_t label_condition  = 0;
    size_t label_body       = 0;
    size_t label_else       = 0;
    size_t label_end        = 0;

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
        fprintf(out, "PUSH 0\n");
        label_else = USE_LABEL_;
        fprintf(out, "JE :label%zu\n", label_else);
        label_body = USE_LABEL_;
        fprintf(out, "JMP :label%zu\n", label_body);
    }

    label_condition = USE_LABEL_;
    fprintf(out, ":label%zu\n", label_condition);
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    fprintf(out, "PUSH 0\n");
    label_end = USE_LABEL_;
    fprintf(out, "JE :label%zu\n", label_end);
    fprintf(out, ":label%zu\n", label_body);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->lt, out));
    }
    else
    {
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));
    }
    fprintf(out, "JMP :label%zu\n", label_condition);

    if (elem->rt->lexem.type == LEXEM_TYPE_OP && elem->rt->lexem.data.op == OP_TYPE_ELSE)
    {
        fprintf(out, ":label%zu\n", label_else);
        TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt->rt, out));
    }

    fprintf(out, ":label%zu\n", label_end);

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "POW\n");

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "ADD\n");

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));


    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "SUB\n");

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "MUL\n");

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    lassert(elem->lt->lexem.type == LEXEM_TYPE_VAR, "");

    size_t* var = NULL;
    CHECK_DECLD_VAR_(var, elem->lt);

    fprintf(out, "PUSH [%ld]\n", VAR_IND_(var));

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "DIV\n");

    fprintf(out, "POP [%ld]\n", VAR_IND_(var));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "EQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "NEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "LESS\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "LEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "GREAT\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->lt, out));
    TRANSLATION_ERROR_HANDLE(translate_recursive_(translator, elem->rt, out));

    fprintf(out, "GEQ\n");

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");

    fprintf(stderr, "Invalid op type: %s\n", __func__);

    return TRANSLATION_ERROR_INVALID_OP_TYPE;
}
