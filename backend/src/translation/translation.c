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
} translator_t;

enum TranslationError translator_ctor_(translator_t* const translator);
void                  translator_dtor_(translator_t* const translator);

enum TranslationError translator_ctor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    STACK_ERROR_HANDLE_(STACK_CTOR(&translator->vars, sizeof(size_t), 10));

    return TRANSLATION_ERROR_SUCCESS;
}

void translator_dtor_(translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    stack_dtor(&translator->vars);
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

    fprintf(out, "HLT\n");

    return TRANSLATION_ERROR_SUCCESS;
}

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case num_: TRANSLATION_ERROR_HANDLE(translate_##name_(translator, elem, out)); break;

enum TranslationError translate_recursive_(translator_t* const translator, const tree_elem_t* elem, 
                                           FILE* out)
{
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
        if (!(finded_var = (size_t*)stack_find(translator->vars, &elem->lexem.data.var)))
        {
            fprintf(stderr, "Use undeclarated var with %zu num\n", elem->lexem.data.var);
            return TRANSLATION_ERROR_UNDECL_VAR;
        }
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



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_MUL(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DIV(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_POW(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_PLEASE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DECL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DECL_FLAG(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_IF(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_RBODY(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_COND_LBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_COND_RBRAKET(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_WHILE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_POW_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_SUM_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_SUB_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_MUL_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_DIV_ASSIGNMENT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_EQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_NEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LESS(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_LESSEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_GREAT(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_GREATEQ(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError translate_ELSE(translator_t* const translator, const tree_elem_t* elem, FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");



    return TRANSLATION_ERROR_SUCCESS;
}
