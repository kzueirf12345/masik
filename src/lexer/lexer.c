#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

#include "lexer/lexer.h"
#include "logger/liblogger.h"
#include "utils/utils.h"
#include "stack_on_array/libstack.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* lexer_strerror(const enum LexerError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(LEXER_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_STANDARD_ERRNO);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_STACK);
        CASE_ENUM_TO_STRING_(LEXER_ERROR_INVALID_LEXEM);

        default:
            return "UNKNOWN_LEXER_ERROR";
    }
    return "UNKNOWN_LEXER_ERROR";
}
#undef CASE_ENUM_TO_STRING_

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


const char* lexem_type_to_str(const enum LexemType type)
{
    switch (type)
    {
        case LEXEM_TYPE_NUM: return "NUM";
        case LEXEM_TYPE_OP:  return "OP";
        case LEXEM_TYPE_END: return "END";
    default:
        return "UNKNOWN_LEXEM_TYPE";
    }
    return "UNKNOWN_LEXEM_TYPE";
}

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

    size_t line = 1;
    for (size_t ind = 0; text[ind] != L'\0'; ++ind)
    {
        if (iswspace((wint_t)text[ind]))
        {
            line += (text[ind] == L'\n');
            continue;
        }

        if (iswdigit((wint_t)text[ind]))
        {
            num_t num = 0;
            for(; ind < text_size && iswdigit((wint_t)text[ind]); ++ind)
            {
                num = num * 10 + (text[ind] - L'0');
            }
            LEXER_ERROR_HANDLE(
                lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_NUM, .data = {.num = num}})
            );
            --ind;
            continue;
        }

        enum OpType op = OP_TYPE_UNKNOWN;
        if ((op = find_op(text + ind)) != OP_TYPE_UNKNOWN)
        {
            LEXER_ERROR_HANDLE(
                lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_OP, .data = {.op = op}})
            );
            ind += wcslen(OPERATIONS[op].keyword) - 1;
            continue;
        }

        fprintf(stderr, "Unknown lexem: '%lc' on %zu line\n", (wint_t)text[ind], line);
        free(text); text = NULL;
        return LEXER_ERROR_INVALID_LEXEM;
    }

    LEXER_ERROR_HANDLE(
        lexer_push(lexer, (lexem_t){.type = LEXEM_TYPE_END, .data = {}})
    );

    free(text); text = NULL;

    return LEXER_ERROR_SUCCESS;
}