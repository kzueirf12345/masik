#include <stdio.h>
#include <string.h>

#include "logger/liblogger.h"
#include "utils/utils.h"
#include "syntaxer/verification/verification.h"
#include "syntaxer/funcs/funcs.h"
#include "stack_on_array/libstack.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        enum StackError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            stack_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return SYNTAX_ERROR_STACK;                                                              \
        }                                                                                           \
    } while(0)

syntax_elem_t* syntax_elem_ctor(lexem_t lexem, syntax_elem_t* lt, syntax_elem_t* rt)
{
    syntax_elem_t* elem = calloc(1, sizeof(syntax_elem_t));

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

static size_t size_ = 0;

void syntax_elem_dtor(syntax_elem_t** elem)
{
    lassert(!is_invalid_ptr(elem), "");

    if (!*elem) return;

    lassert(!is_invalid_ptr(*elem), "");

    --size_; //TODO как по-другому

    free(*elem); *elem = NULL;
}

void syntax_elem_dtor_recursive(syntax_elem_t** elem)
{
    lassert(!is_invalid_ptr(elem), "");

    if (!*elem) return;

    lassert(!is_invalid_ptr(*elem), "");

    syntax_elem_dtor_recursive(&(*elem)->lt);
    syntax_elem_dtor_recursive(&(*elem)->rt);

    syntax_elem_dtor(elem);
}


typedef struct DescState
{
    size_t ind;
    const lexer_t lexer;
    stack_key_t errors;
} desc_state_t;

#define ERROR_MSG_MAX_ 1024


syntax_elem_t* desc_start_      (desc_state_t* const desc_state);

syntax_elem_t* desc_statement_  (desc_state_t* const desc_state);

syntax_elem_t* desc_expr_       (desc_state_t* const desc_state);
syntax_elem_t* desc_assignment_ (desc_state_t* const desc_state);
syntax_elem_t* desc_sum_        (desc_state_t* const desc_state);
syntax_elem_t* desc_mul_        (desc_state_t* const desc_state);
syntax_elem_t* desc_pow_        (desc_state_t* const desc_state);
syntax_elem_t* desc_brakets_    (desc_state_t* const desc_state);
syntax_elem_t* desc_varnum_     (desc_state_t* const desc_state);

syntax_elem_t* desc_declaration_(desc_state_t* const desc_state);

// syntax_elem_t* desc_if_         (desc_state_t* const desc_state);
// syntax_elem_t* desc_condition_  (desc_state_t* const desc_state);
// syntax_elem_t* desc_body_       (desc_state_t* const desc_state);


enum SyntaxError output_errors_(desc_state_t desc_state);

enum SyntaxError syntaxer_ctor(syntaxer_t* const syntaxer, const lexer_t lexer)
{
    lassert(!is_invalid_ptr(syntaxer), "");

    size_ = 0;

    desc_state_t desc_state = {.ind = 0, .lexer = lexer, .errors = 0};
    STACK_ERROR_HANDLE_(STACK_CTOR(&desc_state.errors, ERROR_MSG_MAX_, 0));

    syntaxer->Groot = desc_start_(&desc_state);

    if (!stack_is_empty(desc_state.errors))
    {
        SYNTAX_ERROR_HANDLE(output_errors_(desc_state),              
                                                                     stack_dtor(&desc_state.errors);
        );
                                                                     stack_dtor(&desc_state.errors);
        return SYNTAX_ERROR_SYNTAX_ERROR;
    }

    syntaxer->size = size_;
                                                                     stack_dtor(&desc_state.errors);

    SYNTAXER_VERIFY(syntaxer);
    return SYNTAX_ERROR_SUCCESS;
}

enum SyntaxError output_errors_(desc_state_t desc_state)
{
    fprintf(stderr, RED_TEXT("DESC ERRORS: \n"));

    while (!stack_is_empty(desc_state.errors))
    {
        char error_msg[ERROR_MSG_MAX_] = {};

        STACK_ERROR_HANDLE_(stack_pop(&desc_state.errors, error_msg));

        fprintf(stderr, "%s\n", error_msg);
    }

    return SYNTAX_ERROR_SUCCESS;
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
        snprintf(error_msg, ERROR_MSG_MAX_, "Can't %s line: %d\n", __func__, __LINE__);             \
        STACK_ERROR_HANDLE_(stack_push(&desc_state->errors, error_msg));                            \
        return NULL;                                                                                \
    } while(0)

#define CHECK_ERROR_(...)                                                                           \
        do {                                                                                        \
            lassert(!is_invalid_ptr(desc_state), "");                                               \
            if (IS_FAILURE_) { RET_FAILURE_(__VA_ARGS__); }                                         \
        } while(0)


#define CREATE_ELEM_(lex, lt, rt) (++size_, syntax_elem_ctor(lex, lt, rt))

#define IS_OP_  (CUR_LEX_.type == LEXEM_TYPE_OP)
#define IS_NUM_ (CUR_LEX_.type == LEXEM_TYPE_NUM)
#define IS_VAR_ (CUR_LEX_.type == LEXEM_TYPE_VAR)

// #define DEF_OPERATION_HANDLE

// #define OPERATION_HANDLE(num_, name_, keyword_, ...)                                             
//         IS_##name_##_ (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_##name_)

// #include "operations/codegen.h"

// #undef OPERATION_HANDLE
// #undef DEF_OPERATION_HANDLE

#define IS_OP_TYPE_(name_) (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_##name_)

// #define IS_SUM_     (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_SUM)
// #define IS_SUB_     (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_SUB)
// #define IS_MUL_     (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_MUL)
// #define IS_DIV_     (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_DIV)
// #define IS_POW_     (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_POW)
// #define IS_LBRAKET_ (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_LBRAKET)
// #define IS_RBRAKET_ (IS_OP_ && CUR_LEX_.data.op == OP_TYPE_RBRAKET)

// #define ARG_WITH_COMMA(...) __VA_ARGS__

syntax_elem_t* desc_start_  (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    syntax_elem_t* elem = desc_statement_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    size_t old_ind = CUR_IND_;

    syntax_elem_t* elem2 = desc_statement_(desc_state);

    while (!IS_FAILURE_)
    {
        const lexem_t lexem = {.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_PLEASE}};
        elem = CREATE_ELEM_(lexem, elem, elem2);

        old_ind = CUR_IND_;

        elem2 = desc_statement_(desc_state);
    }
    RESET_ERRORS_;
    CUR_IND_ = old_ind;
    syntax_elem_dtor_recursive(&elem2);

    if (CUR_LEX_.type != LEXEM_TYPE_END)
    {
        fprintf(stderr, "desc_start fail\n");
        RET_FAILURE_(syntax_elem_dtor_recursive(&elem););
    }

    return elem;
}

syntax_elem_t* desc_statement_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    size_t old_ind = CUR_IND_;

    syntax_elem_t* elem = desc_declaration_(desc_state);

    if (IS_FAILURE_)
    {
        RESET_ERRORS_;
        CUR_IND_ = old_ind;
        syntax_elem_dtor_recursive(&elem);

        elem = desc_expr_(desc_state);

        if (IS_FAILURE_)
        {
            RESET_ERRORS_;
            CUR_IND_ = old_ind;
            syntax_elem_dtor_recursive(&elem);
        }
    }

    bool is_please = false;

    while (IS_OP_TYPE_(PLEASE))
    {
        is_please = true;
        SHIFT_;
    }

    if (!is_please)
    {
        RET_FAILURE_(syntax_elem_dtor_recursive(&elem););
    }

    return elem;
}

syntax_elem_t* desc_expr_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    syntax_elem_t* elem = desc_assignment_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    return elem;
}

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

syntax_elem_t* desc_assignment_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    syntax_elem_t* elem = desc_sum_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    while (IS_OP_TYPE_(ASSIGNMENT) || IS_OP_TYPE_(DECL_ASSIGNMENT))
    {
        lexem_t lexem = CUR_LEX_;
        lexem.data.op = OP_TYPE_ASSIGNMENT;
        SHIFT_;

        syntax_elem_t* elem2 = desc_assignment_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

        switch(lexem.data.op)
        {
            #include "operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(syntax_elem_dtor_recursive(&elem); syntax_elem_dtor_recursive(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

syntax_elem_t* desc_sum_(desc_state_t* const desc_state) {
    CHECK_ERROR_();
    
    syntax_elem_t* elem = desc_mul_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    while (IS_OP_TYPE_(SUM) || IS_OP_TYPE_(SUB))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        syntax_elem_t* elem2 = desc_mul_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

        switch(lexem.data.op)
        {
            #include "operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(syntax_elem_dtor_recursive(&elem); syntax_elem_dtor_recursive(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

syntax_elem_t* desc_mul_    (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    syntax_elem_t* elem = desc_pow_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    while (IS_OP_TYPE_(MUL) || IS_OP_TYPE_(DIV))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        syntax_elem_t* elem2 = desc_pow_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

        switch(lexem.data.op)
        {
            #include "operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(syntax_elem_dtor_recursive(&elem); syntax_elem_dtor_recursive(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

#define OPERATION_HANDLE(num_, name_, keyword_, ...)                                                \
        case OP_TYPE_##name_: elem = CREATE_ELEM_(lexem, elem, elem2); break;

syntax_elem_t* desc_pow_    (desc_state_t* const desc_state)
{
    CHECK_ERROR_();
    
    syntax_elem_t* elem = desc_brakets_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    while (IS_OP_TYPE_(POW))
    {
        const lexem_t lexem = CUR_LEX_;
        SHIFT_;

        syntax_elem_t* elem2 = desc_pow_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

        switch(lexem.data.op)
        {
            #include "operations/codegen.h"

            case OP_TYPE_UNKNOWN:
            default:
                RET_FAILURE_(syntax_elem_dtor_recursive(&elem); syntax_elem_dtor_recursive(&elem2););
        }
    }

    return elem;
}
#undef OPERATION_HANDLE

syntax_elem_t* desc_brakets_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (IS_OP_TYPE_(LBRAKET))
    {
        SHIFT_;

        syntax_elem_t* elem = desc_expr_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

        if (!IS_OP_TYPE_(RBRAKET))
        {
            RET_FAILURE_(syntax_elem_dtor_recursive(&elem););
        }
        SHIFT_;

        return elem;
    }

    syntax_elem_t* elem = desc_varnum_(desc_state);
    CHECK_ERROR_(syntax_elem_dtor_recursive(&elem););

    return elem;
}

syntax_elem_t* desc_varnum_(desc_state_t* const desc_state)
{
    CHECK_ERROR_();

    if (!IS_VAR_ && !IS_NUM_)
    {
        RET_FAILURE_();
    }

    syntax_elem_t* elem = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    return elem;
}

syntax_elem_t* desc_declaration_(desc_state_t* const desc_state) 
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
    syntax_elem_t* elem_lt = CREATE_ELEM_(CUR_LEX_, NULL, NULL);
    SHIFT_;

    const lexem_t lexem_zero = {.type = LEXEM_TYPE_NUM, .data = {.num = 0}};
    syntax_elem_t* elem_rt = CREATE_ELEM_(lexem_zero, NULL, NULL);

    lexem_t lexem = CUR_LEX_;
    lexem.data.op = OP_TYPE_DECL_ASSIGNMENT;
    if (IS_OP_TYPE_(ASSIGNMENT) || IS_OP_TYPE_(DECL_ASSIGNMENT))
    {
        SHIFT_;

        syntax_elem_dtor_recursive(&elem_rt);
        elem_rt = desc_expr_(desc_state);
        CHECK_ERROR_(syntax_elem_dtor_recursive(&elem_lt); syntax_elem_dtor_recursive(&elem_rt););
    }

    return CREATE_ELEM_(lexem, elem_lt, elem_rt);
}

void syntaxer_dtor(syntaxer_t* const syntaxer)
{
    SYNTAXER_VERIFY(syntaxer);

    syntax_elem_dtor_recursive(&syntaxer->Groot);
    syntaxer->size = 0;
}