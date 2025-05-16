#include <stdio.h>
#include <locale.h>

#include "utils/utils.h"
#include "logger/liblogger.h"
#include "flags/flags.h"
#include "utils/utils.h"
#include "translation/funcs/funcs.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv);
int dtor_all(flags_objs_t* const flags_objs);

int main(const int argc, char* const argv[])
{
    fprintf(stderr, GREEN_TEXT("Hello ir_backend\n"));

    flags_objs_t flags_objs = {};

    if (init_all(&flags_objs, argc, argv))
    {
        fprintf(stderr, "Can't init all\n");
        return EXIT_FAILURE;
    }

    tree_t tree = {};
    TREE_ERROR_HANDLE(tree_ctor(&tree, flags_objs.in_filename),
                                                                              dtor_all(&flags_objs);
    );

    IR_TRANSLATION_ERROR_HANDLE(translate(&tree, flags_objs.out),
                                                             tree_dtor(&tree);dtor_all(&flags_objs);
    );


    tree_dtor(&tree);
    
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
    lassert(!is_invalid_ptr(flags_objs), "");
    lassert(argc, "");
    lassert(argv, "");

    FLAGS_ERROR_HANDLE(flags_objs_ctor (flags_objs));
    FLAGS_ERROR_HANDLE(flags_processing(flags_objs, argc, argv),      flags_objs_dtor(flags_objs););

    if (logger_init(flags_objs->log_folder))
    {
                                                                        flags_objs_dtor(flags_objs);
        fprintf(stderr, "Can't logger init\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int dtor_all(flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    LOGG_ERROR_HANDLE(                                                               logger_dtor());
    TREE_DUMB_ERROR_HANDLE(                                                       tree_dumb_dtor());
    FLAGS_ERROR_HANDLE(                                                flags_objs_dtor(flags_objs));

    return EXIT_SUCCESS;
}

#define LOGOUT_FILENAME "logout.log"
#define   DUMB_FILENAME "dumb"
int logger_init(char* const log_folder)
{
    lassert(!is_invalid_ptr(log_folder), "");

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
    LOGG_ERROR_HANDLE(logger_set_level_details(LOG_LEVEL_DETAILS_ALL),              logger_dtor(););
    LOGG_ERROR_HANDLE(logger_set_logout_file(logout_filename),                      logger_dtor(););

    TREE_DUMB_ERROR_HANDLE(tree_dumb_ctor(),                                        logger_dtor(););
    TREE_DUMB_ERROR_HANDLE(tree_dumb_set_out_file(dumb_filename),  tree_dumb_dtor();logger_dtor(););
    
    return EXIT_SUCCESS;
}
#undef LOGOUT_FILENAME
#undef   DUMB_FILENAME