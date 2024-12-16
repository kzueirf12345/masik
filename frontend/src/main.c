#include <stdio.h>
#include <locale.h>

#include "logger/liblogger.h"
#include "flags/flags.h"
#include "lexer/funcs/funcs.h"
#include "lexer/verification/verification.h"
#include "syntaxer/funcs/funcs.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv);
int dtor_all(flags_objs_t* const flags_objs);

int main(const int argc, char* const argv[])
{
    fprintf(stderr, GREEN_TEXT("Hello frontend\n"));

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

    tree_t syntaxer;
    TREE_ERROR_HANDLE(syntaxer_ctor(&syntaxer, lexer),
                                                           lexer_dtor(&lexer);dtor_all(&flags_objs);
    );

    // tree_dumb(&syntaxer);

    TREE_ERROR_HANDLE(tree_print(syntaxer, flags_objs.out),
                                      lexer_dtor(&lexer);dtor_all(&flags_objs);tree_dtor(&syntaxer);
    );

    lexer_dtor(&lexer);
    tree_dtor(&syntaxer);
    
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
    TREE_DUMB_ERROR_HANDLE(                                               tree_dumb_dtor());
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

    TREE_DUMB_ERROR_HANDLE(tree_dumb_ctor());
    TREE_DUMB_ERROR_HANDLE(tree_dumb_set_out_file(dumb_filename));
    
    return EXIT_SUCCESS;
}
#undef LOGOUT_FILENAME
#undef   DUMB_FILENAME