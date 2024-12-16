#include <stdio.h>
#include <string.h>

#include "logger/liblogger.h"
#include "utils/utils.h"
#include "tree/verification/verification.h"
#include "tree/funcs/funcs.h"
#include "stack_on_array/libstack.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        enum StackError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            stack_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return TREE_ERROR_STACK;                                                              \
        }                                                                                           \
    } while(0)

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

void tree_dtor(tree_t* const syntaxer)
{
    TREE_VERIFY(syntaxer);

    tree_elem_dtor_recursive(&syntaxer->Groot);
    syntaxer->size = 0;
}