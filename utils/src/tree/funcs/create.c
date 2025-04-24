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


static size_t size_ = 0;

tree_elem_t* tree_ctor_recursive_(wchar_t** token, wchar_t** buffer);

enum TreeError tree_ctor(tree_t* tree, const char* const filename)
{
    TREE_VERIFY_ASSERT(tree);
    lassert(!is_invalid_ptr(filename), "");

    wchar_t* text = NULL;
    size_t text_size = 0;
    if (str_from_file(filename, &text, &text_size))
    {
        fprintf(stderr, "Can't str_from_file\n");
        return TREE_ERROR_STANDARD_ERRNO;
    }

    lassert(text[text_size-1] == L' ', "last symbol: '%lc'", (wint_t)text[text_size-1]);

    wchar_t* buffer = NULL;
    wchar_t* token = wcstok(text, L" ", &buffer);

    size_ = 0;
    tree->Groot = tree_ctor_recursive_(&token, &buffer);
    tree->size = size_;

    lassert(token == NULL, "");
    
    free(text); text = NULL;

    return TREE_ERROR_SUCCESS;
}

lexem_t tree_ctor_lexem_(wchar_t** token, wchar_t** buffer);

#define NEXT_TOKEN_                                                                                 \
    do {                                                                                            \
        if (!*token) {fprintf(stderr, "Incorrect tree in file\n"); return NULL;}                    \
        *token = wcstok(NULL, L" ", buffer);                                                        \
    } while (0)

tree_elem_t* tree_ctor_recursive_(wchar_t** token, wchar_t** buffer)
{
    lassert(!is_invalid_ptr(buffer), "");
    lassert(!is_invalid_ptr(token), "");
    lassert(!is_invalid_ptr(*token), "");

    ++size_;

    lexem_t lexem = tree_ctor_lexem_(token, buffer);
    // fprintf(stderr, "token: %ls\n", token);
    
    size_t count = 0;
    swscanf(*token, L"%zu ", &count);
    NEXT_TOKEN_;

    tree_elem_t* lt = NULL;
    tree_elem_t* rt = NULL;

    if (count > 0)
    {
        lt = tree_ctor_recursive_(token, buffer);
        if (!lt)
        {
            return NULL;
        }
    }
    
    if (count > 1)
    {
        rt = tree_ctor_recursive_(token, buffer);
        if (!rt)
        {
            tree_elem_dtor_recursive(&lt);
            return NULL;
        }
    }
    return tree_elem_ctor(lexem, lt, rt);
}

#undef NEXT_TOKEN_
#define NEXT_TOKEN_                                                                                 \
    do {                                                                                            \
        if (!*token) {fprintf(stderr, "Incorrect tree in file\n"); return lexem;}                   \
        *token = wcstok(NULL, L" ", buffer);                                                        \
    } while (0)

lexem_t tree_ctor_lexem_(wchar_t** token, wchar_t** buffer)
{
    lassert(!is_invalid_ptr(buffer), "");
    lassert(!is_invalid_ptr(token), "");
    lassert(!is_invalid_ptr(*token), "");

    lexem_t lexem = {};

    swscanf(*token, L"%d", &lexem.type);
    NEXT_TOKEN_;
    
    switch (lexem.type)
    {
    case LEXEM_TYPE_OP:
    {
        swscanf(*token, L"%d", &lexem.data.op);
        break;
    }

    case LEXEM_TYPE_NUM:
    {
        swscanf(*token, L"%ld", &lexem.data.num);
        break;
    }

    case LEXEM_TYPE_VAR:
    {
        swscanf(*token, L"%zu", &lexem.data.var);
        break;
    }
    
    case LEXEM_TYPE_END:
    default:
        return lexem;
    }

    NEXT_TOKEN_;
    return lexem;
}



void tree_dtor(tree_t* const tree)
{
    TREE_VERIFY_ASSERT(tree);

    tree_elem_dtor_recursive(&tree->Groot);
    tree->size = 0;
}

enum TreeError tree_print_recursive_(const tree_elem_t* elem, FILE* out);
enum TreeError tree_print_lexem_(const lexem_t lexem, FILE* out);

enum TreeError tree_print(const tree_t tree, FILE* out)
{
    TREE_VERIFY_ASSERT(&tree);
    lassert(!is_invalid_ptr(out), "");

    TREE_ERROR_HANDLE(tree_print_recursive_(tree.Groot, out));

    // fprintf(out, "");

    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_print_recursive_(const tree_elem_t* elem, FILE* out)
{
    if (!elem) return TREE_ERROR_SUCCESS;

    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(out), "");
    
    TREE_ERROR_HANDLE(tree_print_lexem_(elem->lexem, out));

    size_t count = (size_t)(elem->lt != NULL) + (size_t)(elem->rt != NULL);
    fprintf(out, "%zu ", count);

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
    
    case LEXEM_TYPE_END:
    default:
        return TREE_ERROR_INVALID_OP_TYPE;
    }

    return TREE_ERROR_SUCCESS;
}

size_t tree_size_(const tree_elem_t* const elem);

void tree_update_size(tree_t* const tree)
{
    lassert(!is_invalid_ptr(tree), "");

    tree->size = tree_size_(tree->Groot);
}

size_t tree_size_(const tree_elem_t* const elem)
{
    if (!elem) return 0;

    lassert(!is_invalid_ptr(elem), "");

    return 1 + tree_size_(elem->lt) + tree_size_(elem->rt);
}