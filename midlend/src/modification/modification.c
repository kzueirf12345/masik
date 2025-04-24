#include <stdbool.h>

#include "modification/modification.h"
#include "logger/liblogger.h"
#include "utils/src/operations/op_math.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* modification_strerror(const enum ModificationError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(MODIFICATION_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(MODIFICATION_ERROR_STANDARD_ERRNO);
        default:
            return "UNKNOWN_MODIFICATION_ERROR";
    }
    return "UNKNOWN_MODIFICATION_ERROR";
}
#undef CASE_ENUM_TO_STRING_


//====================================================================================

enum TreeError tree_simplify_(tree_t* const tree);

enum TreeError tree_modify(tree_t* const tree, const enum Mode mode)
{
    TREE_VERIFY_ASSERT(tree);

    switch (mode)
    {
    case MODE_NOTHING:
        break;

    case MODE_SIMPLIFY:
        TREE_ERROR_HANDLE(tree_simplify_(tree));
        break;

    case MODE_COMPLICATE:
        //TODO implement
        break;

    case MODE_OBFUSCATE:
        //TODO implement
        break;
    
    default:
        fprintf(stderr, "Unknown modification mode");
        return TREE_ERROR_UNKNOWN;
        break;
    }

    TREE_VERIFY_ASSERT(tree);
    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_simplify_constants_(tree_elem_t** elem, size_t* const count_changes);
enum TreeError tree_simplify_trivial_  (tree_elem_t** elem, size_t* const count_changes);

enum TreeError tree_simplify_(tree_t* const tree)
{
    TREE_VERIFY_ASSERT(tree);

    size_t count_changes = 0;
    do
    {
        count_changes = 0;
        TREE_ERROR_HANDLE(tree_simplify_constants_(&tree->Groot, &count_changes));
        TREE_ERROR_HANDLE(tree_simplify_trivial_  (&tree->Groot, &count_changes));
    } while (count_changes);

    tree_update_size(tree);

    TREE_VERIFY_ASSERT(tree);
    return TREE_ERROR_SUCCESS;
}

#define OPERATION_HANDLE(num_, name_, ...)                                                          \
    case OP_TYPE_##name_:                                                                           \
        *elem = tree_elem_ctor((lexem_t){.type = LEXEM_TYPE_NUM,                                    \
                                         .data.num = math_##name_((*elem)->lt->lexem.data.num,      \
                                                                  (*elem)->rt->lexem.data.num)},    \
                                NULL, NULL);                                                        \
        break;

enum TreeError tree_simplify_constants_(tree_elem_t** elem, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if (!*elem || (*elem)->lexem.type != LEXEM_TYPE_OP)
        return TREE_ERROR_SUCCESS;
    
    TREE_ERROR_HANDLE(tree_simplify_constants_(&(*elem)->lt, count_changes));
    TREE_ERROR_HANDLE(tree_simplify_constants_(&(*elem)->rt, count_changes));


    if (((*elem)->lt && ((*elem)->lt->lexem.type != LEXEM_TYPE_NUM))
     || ((*elem)->rt && ((*elem)->rt->lexem.type != LEXEM_TYPE_NUM))
     || !OPERATIONS[(*elem)->lexem.data.op].is_ariphmetic)
        return TREE_ERROR_SUCCESS;
    

    tree_elem_t* temp = *elem;

    switch((*elem)->lexem.data.op)
    {
        #include "utils/src/operations/codegen.h"

        case OP_TYPE_UNKNOWN:
        default:
            fprintf(stderr, "Unknown op type\n");
            return TREE_ERROR_INVALID_OP_TYPE;
    }

    if ((*elem)->lexem.data.num == NUM_POISON)
    {
        fprintf(stderr, "Incorrect operation\n");
        return TREE_ERROR_INVALID_DATA_NUM;
    }

    tree_elem_dtor_recursive(&temp);

    ++*count_changes;

    return TREE_ERROR_SUCCESS;
}
#undef OPERATION_HANDLE

enum TreeError tree_simplify_POW_(tree_elem_t** tree, size_t* const count_changes);
enum TreeError tree_simplify_MUL_(tree_elem_t** tree, size_t* const count_changes);
enum TreeError tree_simplify_SUM_(tree_elem_t** tree, size_t* const count_changes);
enum TreeError tree_simplify_SUB_(tree_elem_t** tree, size_t* const count_changes);
enum TreeError tree_simplify_DIV_(tree_elem_t** tree, size_t* const count_changes);

enum TreeError tree_simplify_trivial_  (tree_elem_t** elem, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(elem), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if (!*elem || (*elem)->lexem.type != LEXEM_TYPE_OP)
        return TREE_ERROR_SUCCESS;
        
    
    TREE_ERROR_HANDLE(tree_simplify_trivial_(&(*elem)->lt, count_changes));
    TREE_ERROR_HANDLE(tree_simplify_trivial_(&(*elem)->rt, count_changes));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

    switch((*elem)->lexem.data.op)
    {
        case OP_TYPE_POW:
        {
            TREE_ERROR_HANDLE(tree_simplify_POW_(elem, count_changes));
            break;
        }
        case OP_TYPE_MUL:
        {
            TREE_ERROR_HANDLE(tree_simplify_MUL_(elem, count_changes));
            break;
        }
        case OP_TYPE_SUM:
        {
            TREE_ERROR_HANDLE(tree_simplify_SUM_(elem, count_changes));
            break;
        }
        case OP_TYPE_SUB:
        {
            TREE_ERROR_HANDLE(tree_simplify_SUB_(elem, count_changes));
            break;
        }
        case OP_TYPE_DIV:
        {
            TREE_ERROR_HANDLE(tree_simplify_DIV_(elem, count_changes));
            break;
        }
        default:
            break;
    }

#pragma GCC diagnostic pop

    return TREE_ERROR_SUCCESS;
}

static bool is_zero_rt_(const tree_elem_t* const tree)
{
    return tree->rt->lexem.type == LEXEM_TYPE_NUM && tree->rt->lexem.data.num == 0;
}
static bool is_zero_lt_(const tree_elem_t* const tree)
{
    return tree->lt->lexem.type == LEXEM_TYPE_NUM && tree->lt->lexem.data.num == 0;
}
static bool is_one_rt_ (const tree_elem_t* const tree)
{
    return tree->rt->lexem.type == LEXEM_TYPE_NUM && tree->rt->lexem.data.num == 1;
}
static bool is_one_lt_ (const tree_elem_t* const tree)
{
    return tree->lt->lexem.type == LEXEM_TYPE_NUM && tree->lt->lexem.data.num == 1;
}

void change_tree_to_lt_ (tree_elem_t** tree);
void change_tree_to_rt_ (tree_elem_t** tree);
void change_tree_to_num_(tree_elem_t** tree, const num_t num);


enum TreeError tree_simplify_POW_(tree_elem_t** tree, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(tree), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if ((is_one_rt_(*tree)))
    {
        change_tree_to_lt_(tree);
        ++*count_changes;
    }
    else if ((is_zero_rt_(*tree)))
    {
        change_tree_to_num_(tree, 1);
        ++*count_changes;
    }
    
    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_simplify_MUL_(tree_elem_t** tree, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(tree), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if ((is_zero_lt_(*tree)) || (is_zero_rt_(*tree)))
    {
        change_tree_to_num_(tree, 0);
        ++*count_changes;
    }
    else if ((is_one_rt_(*tree)))
    {
        change_tree_to_lt_(tree);
        ++*count_changes;
    }
    else if ((is_one_lt_(*tree)))
    {
        change_tree_to_rt_(tree);
        ++*count_changes;
    }

    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_simplify_SUM_(tree_elem_t** tree, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(tree), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if ((is_zero_rt_(*tree)))
    {
        change_tree_to_lt_(tree);
        ++*count_changes;
    }
    else if ((is_zero_lt_(*tree)))
    {
        change_tree_to_rt_(tree);
        ++*count_changes;
    }

    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_simplify_SUB_(tree_elem_t** tree, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(tree), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if ((is_zero_rt_(*tree)))
    {
        change_tree_to_lt_(tree);
        ++*count_changes;
    }
    else if ((is_zero_lt_(*tree)))
    {
        (*tree)->lexem.data.op = OP_TYPE_MUL;
        (*tree)->lt->lexem.data.num = -1;
        ++*count_changes;
    }

    return TREE_ERROR_SUCCESS;
}

enum TreeError tree_simplify_DIV_(tree_elem_t** tree, size_t* const count_changes)
{
    lassert(!is_invalid_ptr(tree), "");
    lassert(!is_invalid_ptr(count_changes), "");

    if ((is_zero_lt_(*tree)))
    {
        change_tree_to_num_(tree, 0);
        ++*count_changes;
    }

    return TREE_ERROR_SUCCESS;
}

void change_tree_to_lt_ (tree_elem_t** tree)
{
    lassert(!is_invalid_ptr(tree), "");

    tree_elem_t* temp = *tree;

    *tree = (*tree)->lt;

    temp->lt = NULL;
    tree_elem_dtor_recursive(&temp);
}

void change_tree_to_rt_ (tree_elem_t** tree)
{
    lassert(!is_invalid_ptr(tree), "");

    tree_elem_t* temp = *tree;

    *tree = (*tree)->rt;

    temp->rt = NULL;
    tree_elem_dtor_recursive(&temp);
}

void change_tree_to_num_(tree_elem_t** tree, const num_t num)
{
    lassert(!is_invalid_ptr(tree), "");

    tree_elem_t* temp = *tree;

    *tree = tree_elem_ctor((lexem_t){.type = LEXEM_TYPE_NUM, .data.num = num}, NULL, NULL);

    tree_elem_dtor_recursive(&temp);
}


