#include <stdio.h>
#include <string.h>

#include "logger/liblogger.h"
#include "utils/utils.h"
#include "utils/src/tree/verification/verification.h"
#include "syntaxer/funcs/funcs.h"
#include "stack_on_array/libstack.h"
#include "lexer/funcs/funcs.h"

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


typedef struct DescState
{
    size_t ind;
    const lexer_t lexer;
    stack_key_t errors;
} desc_state_t;

#define ERROR_MSG_MAX_ 1024


tree_elem_t* desc_start_        (desc_state_t* const desc_state);

tree_elem_t* desc_func_         (desc_state_t* const desc_state);
tree_elem_t* desc_main_         (desc_state_t* const desc_state);
tree_elem_t* desc_args_         (desc_state_t* const desc_state);
tree_elem_t* desc_vars_         (desc_state_t* const desc_state);

tree_elem_t* desc_statement_    (desc_state_t* const desc_state);

tree_elem_t* desc_expr_         (desc_state_t* const desc_state);
tree_elem_t* desc_assignment_   (desc_state_t* const desc_state);
tree_elem_t* desc_compare_eq_   (desc_state_t* const desc_state);
tree_elem_t* desc_compare_      (desc_state_t* const desc_state);
tree_elem_t* desc_sum_          (desc_state_t* const desc_state);
tree_elem_t* desc_mul_          (desc_state_t* const desc_state);
tree_elem_t* desc_pow_          (desc_state_t* const desc_state);
tree_elem_t* desc_brakets_      (desc_state_t* const desc_state);
tree_elem_t* desc_var_num_func_ (desc_state_t* const desc_state);

tree_elem_t* desc_call_func_    (desc_state_t* const desc_state);
tree_elem_t* desc_ret_          (desc_state_t* const desc_state);

tree_elem_t* desc_declaration_  (desc_state_t* const desc_state);

tree_elem_t* desc_if_           (desc_state_t* const desc_state);
tree_elem_t* desc_while_        (desc_state_t* const desc_state);

tree_elem_t* desc_else_         (desc_state_t* const desc_state);
tree_elem_t* desc_condition_    (desc_state_t* const desc_state);
tree_elem_t* desc_body_         (desc_state_t* const desc_state);


enum TreeError output_errors_(desc_state_t desc_state);

static size_t size_ = 0;

enum TreeError syntaxer_ctor(tree_t* const syntaxer, const lexer_t lexer)
{
    lassert(!is_invalid_ptr(syntaxer), "");

    size_ = 0;

    desc_state_t desc_state = {.ind = 0, .lexer = lexer, .errors = 0};
    STACK_ERROR_HANDLE_(STACK_CTOR(&desc_state.errors, ERROR_MSG_MAX_, 0));

    syntaxer->Groot = desc_start_(&desc_state);

    if (!stack_is_empty(desc_state.errors))
    {
        TREE_ERROR_HANDLE(output_errors_(desc_state),              
                                                                     stack_dtor(&desc_state.errors);
        );
                                                                     stack_dtor(&desc_state.errors);
        return TREE_ERROR_SYNTAX_ERROR;
    }

    syntaxer->size = size_;
                                                                     stack_dtor(&desc_state.errors);

    TREE_VERIFY_ASSERT(syntaxer);
    return TREE_ERROR_SUCCESS;
}

enum TreeError output_errors_(desc_state_t desc_state)
{
    fprintf(stderr, RED_TEXT("DESC ERRORS: \n"));

    while (!stack_is_empty(desc_state.errors))
    {
        char error_msg[ERROR_MSG_MAX_] = {};

        STACK_ERROR_HANDLE_(stack_pop(&desc_state.errors, error_msg));

        fprintf(stderr, "%s\n", error_msg);
    }

    return TREE_ERROR_SUCCESS;
}


#undef  STACK_ERROR_HANDLE_
#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        enum StackError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            stack_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return NULL;                                                                            \
        }                                                                                           \
    } while(0)

#define CUR_IND_ (desc_state->ind)
#define SHIFT_  ++desc_state->ind
#define CUR_LEX_ (*lexer_get(desc_state->lexer, CUR_IND_))

#define IS_FAILURE_ (!stack_is_empty(desc_state->errors))

#define RESET_ERRORS_                                                                               \
        stack_dtor(&desc_state->errors);                                                            \
        STACK_ERROR_HANDLE_(STACK_CTOR(&desc_state->errors, ERROR_MSG_MAX_, 0))

#define RET_FAILURE_(...)                                                                           \
    do {                                                                                            \
        __VA_ARGS__                                                                                 \
        char error_msg[ERROR_MSG_MAX_] = {};                                                        \
        snprintf(error_msg, ERROR_MSG_MAX_, "Can't %s ind: %zu line: %d\n",                         \
                 __func__, CUR_IND_, __LINE__);                                                     \
        STACK_ERROR_HANDLE_(stack_push(&desc_state->errors, error_msg));                            \
        return NULL;                                                                                \
    } while(0)

#define CHECK_ERROR_(...)                                                                           \
        do {                                                                                        \
            lassert(!is_invalid_ptr(desc_state), "");                                               \
            if (IS_FAILURE_) { RET_FAILURE_(__VA_ARGS__); }                                         \
        } while(0)


#define CREATE_ELEM_(lex, lt, rt) (++size_, tree_elem_ctor(lex, lt, rt))

#define IS_OP_  (CUR_LEX_.type == LEXEM_TYPE_OP)
#define IS_NUM_ (CUR_LEX_.type == LEXEM_TYPE_NUM)
#define IS_VAR_ (CUR_LEX_.type == LEXEM_TYPE_VAR)

#define IS_OP_TYPE_(name_) (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_##name_)
#define IS_ASSIGNMENT (                                                                             \
        IS_OP_TYPE_(    ASSIGNMENT) || IS_OP_TYPE_(DECL_ASSIGNMENT)                                 \
     || IS_OP_TYPE_(SUM_ASSIGNMENT) || IS_OP_TYPE_( SUB_ASSIGNMENT)                                 \
     || IS_OP_TYPE_(MUL_ASSIGNMENT) || IS_OP_TYPE_( DIV_ASSIGNMENT)                                 \
     || IS_OP_TYPE_(POW_ASSIGNMENT)                                                                 \
    )

void tree_elem_dtor_recursive_(tree_elem_t** elem);

tree_elem_t* desc_start_  (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    const lexem_t lexem_please = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_PLEASE}};

    size_t old_ind = CUR_IND_;

    tree_elem_t* func_lt = desc_func_(desc_state);

    if (IS_FAILURE_)
    {
        tree_elem_dtor_recursive_(&func_lt);
    }
    else
    {
        old_ind = CUR_IND_;
        RESET_ERRORS_;
        tree_elem_t* func_lt2 = desc_func_(desc_state);

        while (!IS_FAILURE_)
        {
            func_lt = CREATE_ELEM_(lexem_please, func_lt, func_lt2);

            old_ind = CUR_IND_;
            func_lt2 = desc_func_(desc_state);
        }
        tree_elem_dtor_recursive_(&func_lt2);
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;

    //main

    tree_elem_t* main = desc_main_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&func_lt);tree_elem_dtor_recursive_(&main););

    //func rt

    old_ind = CUR_IND_;

    tree_elem_t* func_rt = desc_func_(desc_state);

    if (IS_FAILURE_)
    {
        tree_elem_dtor_recursive_(&func_rt);
    }
    else
    {
        old_ind = CUR_IND_;
        RESET_ERRORS_;
        tree_elem_t* func_rt2 = desc_func_(desc_state);

        while (!IS_FAILURE_)
        {
            func_rt = CREATE_ELEM_(lexem_please, func_rt, func_rt2);

            old_ind = CUR_IND_;
            func_rt2 = desc_func_(desc_state);
        }
        tree_elem_dtor_recursive_(&func_rt2);
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;

    //all
    lexem_t lexem_main = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_MAIN}};

    if (CUR_LEX_.type != LEXEM_TYPE_END)
    {
        fprintf(stderr, "desc_start fail\n");
        RET_FAILURE_(tree_elem_dtor_recursive_(&func_lt);
                     tree_elem_dtor_recursive_(&func_rt);
                     tree_elem_dtor_recursive_(&main););
    }

    return CREATE_ELEM_(lexem_main, main, CREATE_ELEM_(lexem_please, func_lt, func_rt));
}

tree_elem_t* desc_func_(desc_state_t* const desc_state) 
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(FUNC))
    {
        // fprintf(stderr, "lexem_type: %d, op_type1_func: %s\n", CUR_LEX_.type,
        //         op_type_to_str(CUR_LEX_.data.op));
        RET_FAILURE_();
    }
    lexem_t lexem_func = CUR_LEX_;
    SHIFT_;

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    tree_elem_t* name = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    if (!IS_OP_TYPE_(LBRAKET))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&name););
    }
    SHIFT_;

    size_t old_ind = CUR_IND_;
    tree_elem_t* args = desc_vars_(desc_state);
    if (IS_FAILURE_)
    {
        RESET_ERRORS_;
        CUR_IND_ = old_ind;
        tree_elem_dtor_recursive_(&args);
    }

    if (!IS_OP_TYPE_(RBRAKET))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&name);tree_elem_dtor_recursive_(&args););
    }
    SHIFT_;

    tree_elem_t* body = desc_body_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&name);tree_elem_dtor_recursive_(&args);
                 tree_elem_dtor_recursive_(&body););
    
    lexem_t lexem_please = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_PLEASE}};
    return CREATE_ELEM_(lexem_func, CREATE_ELEM_(lexem_please, name, args), body);
}

tree_elem_t* desc_main_(desc_state_t* const desc_state) 
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(MAIN))
    {
        RET_FAILURE_();
    }
    SHIFT_;

    tree_elem_t* elem = desc_body_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    return elem;
}

tree_elem_t* desc_args_(desc_state_t* const desc_state) 
{
    CHECK_ERROR_();

    tree_elem_t* elem = desc_expr_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while(IS_OP_TYPE_(ARGS_COMMA))
    {
        lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_expr_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem);tree_elem_dtor_recursive_(&elem2););

        elem = CREATE_ELEM_(lexem, elem, elem2);
    }

    return elem;
}

tree_elem_t* desc_vars_(desc_state_t* const desc_state) 
{
    CHECK_ERROR_();

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    tree_elem_t* elem = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    while(IS_OP_TYPE_(ARGS_COMMA))
    {
        lexem_t lexem = CUR_LEX_;
        SHIFT_;

        if (!IS_VAR_)
        {
            RET_FAILURE_();
        }
        tree_elem_t* elem2 = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
        SHIFT_;

        elem = CREATE_ELEM_(lexem, elem, elem2);
    }

    return elem;
}

tree_elem_t* desc_statement_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    size_t old_ind = CUR_IND_;

    tree_elem_t* elem = desc_if_(desc_state);

    if (!IS_FAILURE_)
    {
        return elem;
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;
    tree_elem_dtor_recursive_(&elem);

    elem = desc_while_(desc_state);

    if (!IS_FAILURE_)
    {
        return elem;
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;
    tree_elem_dtor_recursive_(&elem);

    //-------

    old_ind = CUR_IND_;

    elem = desc_declaration_(desc_state);

    if (IS_FAILURE_)
    {
        RESET_ERRORS_;
        CUR_IND_ = old_ind;
        tree_elem_dtor_recursive_(&elem);

        elem = desc_assignment_(desc_state);

        if (IS_FAILURE_)
        {
            RESET_ERRORS_;
            CUR_IND_ = old_ind;
            tree_elem_dtor_recursive_(&elem);

            elem = desc_ret_(desc_state);

                // fprintf(stderr, "op_type: %s\n", op_type_to_str(CUR_LEX_.data.op));

            if (IS_FAILURE_)
            {
                // fprintf(stderr, "kek\n");
                RESET_ERRORS_;
                CUR_IND_ = old_ind;
                tree_elem_dtor_recursive_(&elem);
            }
        }
    }

    bool is_please = false;

    while (IS_OP_TYPE_(PLEASE))
    {
        is_please = true;
        SHIFT_;
        // fprintf(stderr, "kek\n");
    }

    if (!is_please)
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&elem););
    }

    return elem;
}

tree_elem_t* desc_expr_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    tree_elem_t* elem = desc_compare_eq_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    return elem;
}

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_assignment_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    tree_elem_t* elem_name = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    if (!IS_ASSIGNMENT)
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&elem_name););
    }
    lexem_t lexem = CUR_LEX_;
    SHIFT_;

    tree_elem_t* elem_expr = desc_expr_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_expr);tree_elem_dtor_recursive_(&elem_name););

    return CREATE_ELEM_(lexem, elem_name, elem_expr);
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_compare_eq_ (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    tree_elem_t* elem = desc_compare_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while (IS_OP_TYPE_(EQ) || IS_OP_TYPE_(NEQ))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_compare_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        switch(lexem.data.op)
        {
#include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(tree_elem_dtor_recursive_(&elem); tree_elem_dtor_recursive_(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_compare_    (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    tree_elem_t* elem = desc_sum_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while (IS_OP_TYPE_(LESS) || IS_OP_TYPE_(LESSEQ) || IS_OP_TYPE_(GREAT) || IS_OP_TYPE_(GREATEQ))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_sum_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        switch(lexem.data.op)
        {
#include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(tree_elem_dtor_recursive_(&elem); tree_elem_dtor_recursive_(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_sum_(desc_state_t* const desc_state) {
    CHECK_ERROR_();
    
    tree_elem_t* elem = desc_mul_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while (IS_OP_TYPE_(SUM) || IS_OP_TYPE_(SUB))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_mul_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        switch(lexem.data.op)
        {
#include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(tree_elem_dtor_recursive_(&elem); tree_elem_dtor_recursive_(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_mul_    (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    tree_elem_t* elem = desc_pow_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while (IS_OP_TYPE_(MUL) || IS_OP_TYPE_(DIV))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_pow_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        switch(lexem.data.op)
        {
#include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(tree_elem_dtor_recursive_(&elem); tree_elem_dtor_recursive_(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

tree_elem_t* desc_pow_    (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    tree_elem_t* elem = desc_brakets_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    while (IS_OP_TYPE_(POW))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        tree_elem_t* elem2 = desc_pow_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        switch(lexem.data.op)
        {
#include "utils/src/operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(tree_elem_dtor_recursive_(&elem); tree_elem_dtor_recursive_(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

tree_elem_t* desc_brakets_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (IS_OP_TYPE_(LBRAKET))
    {
        SHIFT_;

        tree_elem_t* elem = desc_expr_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

        if (!IS_OP_TYPE_(RBRAKET))
        {
            RET_FAILURE_(tree_elem_dtor_recursive_(&elem););
        }
        SHIFT_;

        return elem;
    }

    tree_elem_t* elem = desc_var_num_func_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    return elem;
}

tree_elem_t* desc_var_num_func_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (IS_NUM_)
    {
        tree_elem_t* elem = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
        SHIFT_;
        return elem;
    }

    size_t old_ind = CUR_IND_;

    tree_elem_t* elem = desc_call_func_(desc_state);
    if (!IS_FAILURE_)
    {
        return elem;
    }
    tree_elem_dtor_recursive_(&elem);
    CUR_IND_ = old_ind;
    RESET_ERRORS_;

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    elem = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;
    
    return elem;
}

tree_elem_t* desc_call_func_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    tree_elem_t* name = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    if (!IS_OP_TYPE_(LBRAKET))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&name););
    }
    SHIFT_;

    size_t old_ind = CUR_IND_;
    tree_elem_t* args = desc_args_(desc_state);
    if (IS_FAILURE_)
    {
        CUR_IND_ = old_ind;
        RESET_ERRORS_;
        tree_elem_dtor_recursive_(&args);
    }

    if (!IS_OP_TYPE_(RBRAKET))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&name);tree_elem_dtor_recursive_(&args););
    }
    SHIFT_;

    lexem_t lexem1 = {.type = LEXEM_TYPE_OP,  .data = {.op = OP_TYPE_FUNC_LBRAKET}};
    return CREATE_ELEM_(lexem1, name, args);
}

tree_elem_t* desc_ret_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();


    if (!IS_OP_TYPE_(RET))
    {
        RET_FAILURE_();
    }
    lexem_t lexem = CUR_LEX_;
    SHIFT_;

    tree_elem_t* elem_lt = desc_expr_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_lt););

    // fprintf(stderr, "op_type1_ret: %s\n", op_type_to_str(CUR_LEX_.data.op));

    return CREATE_ELEM_(lexem, elem_lt, NULL);
}

tree_elem_t* desc_declaration_(desc_state_t* const desc_state) 
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(DECL_FLAG))
    {
        RET_FAILURE_();
    }
    SHIFT_;

    if (!IS_VAR_)
    {
        RET_FAILURE_();
    }
    tree_elem_t* elem_lt = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    const lexem_t lexem_zero = {.type = LEXEM_TYPE_NUM, .data = {.num = 0}};
    tree_elem_t* elem_rt = CREATE_ELEM_(lexem_zero, NULL, NULL);

    lexem_t lexem = CUR_LEX_;
    lexem.data.op = OP_TYPE_DECL_ASSIGNMENT;
    if (IS_OP_TYPE_(ASSIGNMENT) || IS_OP_TYPE_(DECL_ASSIGNMENT))
    {
        SHIFT_;

        tree_elem_dtor_recursive_(&elem_rt);
        elem_rt = desc_expr_(desc_state);
        CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_lt); tree_elem_dtor_recursive_(&elem_rt););
    }

    return CREATE_ELEM_(lexem, elem_lt, elem_rt);
}

tree_elem_t* desc_if_         (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(IF))
    {
        RET_FAILURE_();
    }
    lexem_t lexem_if = CUR_LEX_;
    SHIFT_;

    tree_elem_t* elem_cond = desc_condition_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_cond););

    tree_elem_t* elem_if_body = desc_body_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_if_body); tree_elem_dtor_recursive_(&elem_cond););

    size_t old_ind = CUR_IND_;

    tree_elem_t* elem_else_body = desc_else_(desc_state);
    if (IS_FAILURE_)
    {
        RESET_ERRORS_;
        CUR_IND_ = old_ind;
        tree_elem_dtor_recursive_(&elem_else_body);
    }

    lexem_t lexem_else = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_ELSE}};

    return CREATE_ELEM_(lexem_if, 
                            elem_cond, 
                            CREATE_ELEM_(lexem_else, elem_if_body, elem_else_body));
}

tree_elem_t* desc_while_      (desc_state_t* const desc_state)
{
CHECK_ERROR_();

    if (!IS_OP_TYPE_(WHILE))
    {
        RET_FAILURE_();
    }
    lexem_t lexem_while = CUR_LEX_;
    SHIFT_;

    tree_elem_t* elem_cond = desc_condition_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_cond););

    tree_elem_t* elem_while_body = desc_body_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem_while_body); 
                 tree_elem_dtor_recursive_(&elem_cond););

    size_t old_ind = CUR_IND_;

    tree_elem_t* elem_else_body = desc_else_(desc_state);
    if (IS_FAILURE_)
    {
        RESET_ERRORS_;
        CUR_IND_ = old_ind;
        tree_elem_dtor_recursive_(&elem_else_body);
    }

    lexem_t lexem_else = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_ELSE}};

    return CREATE_ELEM_(lexem_while, 
                            elem_cond, 
                            CREATE_ELEM_(lexem_else, elem_while_body, elem_else_body));
}

tree_elem_t* desc_else_       (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(ELSE))
    {
        RET_FAILURE_();
    }
    SHIFT_;

    tree_elem_t* elem = desc_body_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    return elem;
}

tree_elem_t* desc_condition_  (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(COND_LBRAKET))
    {
        RET_FAILURE_();
    }
    SHIFT_;

    tree_elem_t* elem = desc_expr_(desc_state);
    CHECK_ERROR_(tree_elem_dtor_recursive_(&elem););

    if (!IS_OP_TYPE_(COND_RBRAKET))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&elem););
    }
    SHIFT_;

    return elem;
}

tree_elem_t* desc_body_       (desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_OP_TYPE_(LBODY))
    {
        RET_FAILURE_();
    }
    SHIFT_;

    size_t old_ind = CUR_IND_;
    tree_elem_t* elem = desc_statement_(desc_state);

    if (IS_FAILURE_)
    {
        tree_elem_dtor_recursive_(&elem);
    }
    else
    {
        const lexem_t lexem = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_PLEASE}};

        old_ind = CUR_IND_;
        tree_elem_t* elem2 = desc_statement_(desc_state);
        // fprintf(stderr, "op_type: %s\n", op_type_to_str(CUR_LEX_.data.op));

        while (!IS_FAILURE_)
        {
            elem = CREATE_ELEM_(lexem, elem, elem2);

            old_ind = CUR_IND_;
            elem2 = desc_statement_(desc_state);
        }
        tree_elem_dtor_recursive_(&elem2);
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;

    // fprintf(stderr, "op_type: %s\n", op_type_to_str(CUR_LEX_.data.op));


    if (!IS_OP_TYPE_(RBODY))
    {
        RET_FAILURE_(tree_elem_dtor_recursive_(&elem););
    }
    SHIFT_;

    return elem;
}

void tree_elem_dtor_recursive_(tree_elem_t** elem)
{
    lassert(!is_invalid_ptr(elem), "");

    if (!*elem) return;

    lassert(!is_invalid_ptr(*elem), "");

    tree_elem_dtor_recursive_(&(*elem)->lt);
    tree_elem_dtor_recursive_(&(*elem)->rt);

    tree_elem_dtor(elem);
    --size_;
}