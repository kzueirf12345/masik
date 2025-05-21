#include <stdio.h>
#include <locale.h>

#include "utils/utils.h"
#include "logger/liblogger.h"
#include "flags/flags.h"
#include "utils/utils.h"
#include "translation/funcs/funcs.h"
#include "ir_fist/funcs/funcs.h"
#include "ir_fist/structs.h"

int init_all(flags_objs_t* const flags_objs, const int argc, char* const * argv);
int dtor_all(flags_objs_t* const flags_objs);

int main(const int argc, char* const argv[])
{
    fprintf(stderr, GREEN_TEXT("Hello backend\n"));

    flags_objs_t flags_objs = {};

    if (init_all(&flags_objs, argc, argv))
    {
        fprintf(stderr, "Can't init all\n");
        return EXIT_FAILURE;
    }

    fist_t fist = {};
    FIST_ERROR_HANDLE(FIST_CTOR(&fist, sizeof(ir_block_t), 10),
                                                                              dtor_all(&flags_objs);
    );
    IR_FIST_ERROR_HANDLE(ir_fist_ctor(&fist, flags_objs.in_filename),
                                                             dtor_all(&flags_objs);fist_dtor(&fist);
    );

    TRANSLATION_ERROR_HANDLE(translate_splu(&fist, flags_objs.splu_out),
                                                             dtor_all(&flags_objs);fist_dtor(&fist);
    );

    TRANSLATION_ERROR_HANDLE(translate_nasm(&fist, flags_objs.nasm_out),
                                                             dtor_all(&flags_objs);fist_dtor(&fist);
    );

    TRANSLATION_ERROR_HANDLE(translate_elf(&fist, flags_objs.elf_out),
                                                             dtor_all(&flags_objs);fist_dtor(&fist);
    );


    fist_dtor(&fist);
    
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
    TREE_DUMB_ERROR_HANDLE(                                                       tree_dumb_dtor());
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

    FIST_DUMB_ERROR_HANDLE(fist_dumb_ctor());
    
    return EXIT_SUCCESS;
}
#undef LOGOUT_FILENAME
#undef   DUMB_FILENAME