#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

#include "lexer/funcs/funcs.h"
#include "logger/liblogger.h"
#include "utils/utils.h"
#include "stack_on_array/libstack.h"
#include "lexer/verification/verification.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        enum StackError error_handler = call_func;                                                  \
        if (error_handler)                                                                          \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Error: %s\n",                                     \
                            stack_strerror(error_handler));                                         \
            __VA_ARGS__                                                                             \
            return LEXER_ERROR_STACK;                                                               \
        }                                                                                           \
    } while(0)

#define START_CAPACITY_ 256
enum LexerError lexer_ctor(lexer_t* const lexer)
{
    lassert(!is_invalid_ptr(lexer), "");

    lexer->stack = 0;
    STACK_ERROR_HANDLE_(STACK_CTOR(&lexer->stack, sizeof(lexem_t), START_CAPACITY_));

    return LEXER_ERROR_SUCCESS;
}
#undef START_CAPACITY_

void lexer_dtor(lexer_t* const lexer)
{
    lassert(!is_invalid_ptr(lexer), "");

    stack_dtor(&lexer->stack);
}

enum LexerError lexer_push(lexer_t* const lexer, const lexem_t lexem)
{
    lassert(!is_invalid_ptr(lexer), "");

    STACK_ERROR_HANDLE_(stack_push(&lexer->stack, &lexem));

    return LEXER_ERROR_SUCCESS;
}

lexem_t* lexer_get(lexer_t lexer, const size_t ind)
{
    lassert(ind < stack_size(lexer.stack), "");

    return (lexem_t*)stack_get(lexer.stack, ind);
}

static enum LexerError handle_num_(lexer_t* const lexer, const wchar_t* const text, size_t* const ind);
static enum LexerError handle_var_(lexer_t* const lexer, const wchar_t* const text, size_t* const ind);
static enum LexerError handle_op_ (lexer_t* const lexer,                            size_t* const ind,
                                   const enum OpType op);

static stack_key_t stack = 0;

enum LexerError lexing(lexer_t* const lexer, const char* const filename)
{
    lassert(!is_invalid_ptr(lexer), "");
    lassert(!is_invalid_ptr(filename), "");

    wchar_t* text = NULL;
    size_t text_size = 0;
    if (str_from_file(filename, &text, &text_size))
    {
        fprintf(stderr, "Can't str_from_file\n");
        return LEXER_ERROR_STANDARD_ERRNO;
    }

    lassert(text[text_size-1] == L'\0', "");

    STACK_ERROR_HANDLE_(STACK_CTOR(&stack, VAR_NAME_MAX * sizeof(wchar_t), 10));

    size_t line = 1;
    for (size_t ind = 0; text[ind] != L'\0'; ++ind)
    {
        if (text[ind] == L'\\')
        {
            ++ind;
            for (; text[ind] != L'\\'; ++ind);
            continue;
        }

        if (iswspace((wint_t)text[ind]))
        {
            line += (text[ind] == L'\n');
            continue;
        }

        if (iswdigit((wint_t)text[ind]))
        {
            LEXER_ERROR_HANDLE(handle_num_(lexer, text, &ind), free(text);stack_dtor(&stack););
            continue;
        }

        enum OpType op = OP_TYPE_UNKNOWN;
        if ((op = find_op(text + ind)) != OP_TYPE_UNKNOWN)
        {
            LEXER_ERROR_HANDLE(handle_op_(lexer, &ind, op), free(text);stack_dtor(&stack););
            continue;
        }

        LEXER_ERROR_HANDLE(handle_var_(lexer, text, &ind), free(text);stack_dtor(&stack););
    }

    LEXER_ERROR_HANDLE(
        lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_END, .data = {}}), 
        free(text);stack_dtor(&stack);
    );

    free(text); text = NULL;
    stack_dtor(&stack);

    return LEXER_ERROR_SUCCESS;
}

static enum LexerError handle_num_(lexer_t* const lexer, const wchar_t* const text, size_t* const ind)
{
    lassert(!is_invalid_ptr(lexer), "");
    lassert(!is_invalid_ptr(text),  "");
    lassert(!is_invalid_ptr(ind),   "");

    num_t num = 0;
    for(; text[*ind] != L'\0' && iswdigit((wint_t)text[*ind]); ++*ind)
    {
        num = num * 10 + (text[*ind] - L'0');
    }
    LEXER_ERROR_HANDLE(lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_NUM, .data = {.num = num}}));
    --*ind;

    return LEXER_ERROR_SUCCESS;
}

static enum LexerError handle_var_(lexer_t* const lexer, const wchar_t* const text, size_t* const ind)
{
    lassert(!is_invalid_ptr(lexer), "");
    lassert(!is_invalid_ptr(text),  "");
    lassert(!is_invalid_ptr(ind),   "");

    wchar_t name[VAR_NAME_MAX] = {};

    for (const size_t old_ind = *ind; 
         *ind - old_ind < VAR_NAME_MAX && !iswspace((wint_t)text[*ind]) && text[*ind] != L'\0';
         ++*ind)
    {
        name[*ind - old_ind] = text[*ind];
    }

    const lexem_t lexem = {.type = LEXEM_TYPE_VAR, .data = {.var = stack_find_push(&stack, name)}};
    LEXER_ERROR_HANDLE(lexer_push(lexer, lexem));

    --*ind;

    return LEXER_ERROR_SUCCESS;
}

static enum LexerError handle_op_(lexer_t* const lexer, size_t* const ind, const enum OpType op)
{
    lassert(!is_invalid_ptr(lexer), "");
    lassert(!is_invalid_ptr(ind),   "");

    LEXER_ERROR_HANDLE(lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_OP, .data = {.op = op}}));
    *ind += wcslen(OPERATIONS[op].keyword) - 1;

    return LEXER_ERROR_SUCCESS;
}