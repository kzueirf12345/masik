#include <stdio.h>
#include <string.h>

#include "logger/liblogger.h"
#include "utils/utils.h"
#include "tree/verification/verification.h"
#include "tree/funcs/funcs.h"
#include "stack_on_array/libstack.h"

const char* lexem_type_to_str(const enum LexemType type)
{
    switch (type)
    {
        case LEXEM_TYPE_NUM: return "NUM";
        case LEXEM_TYPE_OP:  return "OP";
        case LEXEM_TYPE_VAR: return "VAR";
        case LEXEM_TYPE_END: return "END";
    default:
        return "UNKNOWN_LEXEM_TYPE";
    }
    return "UNKNOWN_LEXEM_TYPE";
}


tree_elem_t* tree_elem_ctor(lexem_t lexem, tree_elem_t* lt, tree_elem_t* rt)
{
    tree_elem_t* elem = calloc(1, sizeof(tree_elem_t));

    if (!elem)
    {
        perror("Can't calloc elem");
        return NULL;
    }

    elem->lexem = lexem;
    elem->lt    = lt;
    elem->rt    = rt;

    return elem;
}

void tree_elem_dtor(tree_elem_t** elem)
{
    lassert(!is_invalid_ptr(elem), "");

    if (!*elem) return;

    lassert(!is_invalid_ptr(*elem), "");

    free(*elem); *elem = NULL;
}

void tree_elem_dtor_recursive(tree_elem_t** elem)
{
    lassert(!is_invalid_ptr(elem), "");

    if (!*elem) return;

    lassert(!is_invalid_ptr(*elem), "");

    tree_elem_dtor_recursive(&(*elem)->lt);
    tree_elem_dtor_recursive(&(*elem)->rt);

    tree_elem_dtor(elem);
}

enum TreeError tree_ctor(tree_t* tree, const char* const filename)
{
    TREE_VERIFY(tree);
    lassert(!is_invalid_ptr(filename), "");

    wchar_t* text = NULL;
    size_t text_size = 0;
    if (str_from_file(filename, &text, &text_size))
    {
        fprintf(stderr, "Can't str_from_file\n");
        return TREE_ERROR_STANDARD_ERRNO;
    }

    lassert(text[text_size-1] == L'\0', "");

    //TODO implement

    return TREE_ERROR_SUCCESS;
}

void tree_dtor(tree_t* const tree)
{
    TREE_VERIFY(tree);

    tree_elem_dtor_recursive(&tree->Groot);
    tree->size = 0;
}

enum TreeError tree_print_recursive_(const tree_elem_t* elem, FILE* out);
enum TreeError tree_print_lexem_(const lexem_t lexem, FILE* out);

enum TreeError tree_print(const tree_t tree, FILE* out)
{
    TREE_VERIFY(&tree);
    lassert(!is_invalid_ptr(out), "");

    return tree_print_recursive_(tree.Groot, out);
}

enum TreeError tree_print_recursive_(const tree_elem_t* elem, FILE* out)
{
    if (!elem) return TREE_ERROR_SUCCESS;

    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    
    TREE_ERROR_HANDLE(tree_print_lexem_(elem->lexem, out));

    TREE_ERROR_HANDLE(tree_print_recursive_(elem->lt, out));
    TREE_ERROR_HANDLE(tree_print_recursive_(elem->rt, out));

    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_print_lexem_(const lexem_t lexem, FILE* out)
{
    lassert(!is_invalid_ptr(out), "");

    fprintf(out, "%d ", (int)lexem.type);

    switch (lexem.type)
    {
    case LEXEM_TYPE_END:
    {
        fprintf(out, "%ld ", lexem.data.num); // for simplify read
        break;
    }

    case LEXEM_TYPE_OP:
    {
        fprintf(out, "%d ", (int)lexem.data.op);
        break;
    }

    case LEXEM_TYPE_NUM:
    {
        fprintf(out, "%ld ", lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        fprintf(out, "%zu ", lexem.data.var);
        break;
    }
    
    default:
        return TREE_ERROR_INVALID_OP_TYPE;
    }

    return TREE_ERROR_SUCCESS;
}