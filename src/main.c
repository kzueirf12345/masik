#include <stdio.h>
#include <locale.h>

#include "logger/liblogger.h"
#include "flags/flags.h"
#include "lexer/lexer.h"
#include "syntaxer/funcs/funcs.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv);
int dtor_all(flags_objs_t* const flags_objs);

int main(const int argc, char* const argv[])
{
    flags_objs_t flags_objs = {};

    if (init_all(&flags_objs, argc, argv))
    {
        fprintf(stderr, "Can't init all\n");
        return EXIT_FAILURE;
    }

    lexer_t lexer;
    LEXER_ERROR_HANDLE(lexer_ctor(&lexer),
                                                                              dtor_all(&flags_objs);
    );

    LEXER_ERROR_HANDLE(lexing(&lexer, flags_objs.in_filename),
                                                           lexer_dtor(&lexer);dtor_all(&flags_objs);
    );

    syntaxer_t syntaxer;
    SYNTAX_ERROR_HANDLE(syntaxer_ctor(&syntaxer, lexer),
                                                           lexer_dtor(&lexer);dtor_all(&flags_objs);
    );

    // syntaxer_t syntaxer;
    // syntaxer.Groot = syntax_elem_ctor((lexem_t){.type = LEXEM_TYPE_OP, .data = {.op = OP_TYPE_SUM}},
    //                     syntax_elem_ctor((lexem_t){.type = LEXEM_TYPE_NUM, .data = {.num = 1}}, NULL, NULL),
    //                     syntax_elem_ctor((lexem_t){.type = LEXEM_TYPE_NUM, .data = {.num = 2}}, NULL, NULL)
    // );
    // syntaxer.size = 3;

    syntaxer_dumb(&syntaxer);

    lexer_dtor(&lexer);
    syntaxer_dtor(&syntaxer);
    
    if (dtor_all(&flags_objs))
    {
        fprintf(stderr, "Can't dtor all\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int logger_init(char* const log_folder);

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv)
{
    lassert(argc, "");
    lassert(argv, "");

    if (!setlocale(LC_ALL, "ru_RU.utf8"))
    {
        fprintf(stderr, "Can't setlocale\n");
        return EXIT_FAILURE;
    }

    FLAGS_ERROR_HANDLE(flags_objs_ctor (flags_objs));
    FLAGS_ERROR_HANDLE(flags_processing(flags_objs, argc, argv));

    if (logger_init(flags_objs->log_folder))
    {
        fprintf(stderr, "Can't logger init\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int dtor_all(flags_objs_t* const flags_objs)
{
    LOGG_ERROR_HANDLE(                                                               logger_dtor());
    SYNTAXER_DUMB_ERROR_HANDLE(                                               syntaxer_dumb_dtor());
    FLAGS_ERROR_HANDLE(                                                flags_objs_dtor(flags_objs));

    return EXIT_SUCCESS;
}

#define LOGOUT_FILENAME "logout.log"
#define   DUMB_FILENAME "dumb"
int logger_init(char* const log_folder)
{
    lassert(log_folder, "");

    char logout_filename[FILENAME_MAX] = {};
    if (snprintf(logout_filename, FILENAME_MAX, "%s%s", log_folder, LOGOUT_FILENAME) <= 0)
    {
        perror("Can't snprintf logout_filename");
        return EXIT_FAILURE;
    }

    char dumb_filename[FILENAME_MAX] = {};
    if (snprintf(dumb_filename, FILENAME_MAX, "%s%s", log_folder, DUMB_FILENAME) <= 0)
    {
        perror("Can't snprintf dumb_filename");
        return EXIT_FAILURE;
    }

    LOGG_ERROR_HANDLE(logger_ctor());
    LOGG_ERROR_HANDLE(logger_set_level_details(LOG_LEVEL_DETAILS_ALL));
    LOGG_ERROR_HANDLE(logger_set_logout_file(logout_filename));

    SYNTAXER_DUMB_ERROR_HANDLE(syntaxer_dumb_ctor());
    SYNTAXER_DUMB_ERROR_HANDLE(syntaxer_dumb_set_out_file(dumb_filename));
    
    return EXIT_SUCCESS;
}
#undef LOGOUT_FILENAME
#undef   DUMB_FILENAME