#include <string.h>
#include <getopt.h>

#include "flags/flags.h"
#include "logger/liblogger.h"
#include "utils/utils.h"

#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* flags_strerror(const enum FlagsError error)
{
    switch(error)
    {
        CASE_ENUM_TO_STRING_(FLAGS_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(FLAGS_ERROR_FAILURE);
        default:
            return "UNKNOWN_FLAGS_ERROR";
    }
    return "UNKNOWN_FLAGS_ERROR";
}
#undef CASE_ENUM_TO_STRING_


enum FlagsError flags_objs_ctor(flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    if (!strncpy(flags_objs->log_folder, "./log/", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->log_folder");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->in_filename, "../assets/input.msk", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->in_filename");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->out_filename, "../assets/front_out.txt", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->in_filename");
        return FLAGS_ERROR_SUCCESS;
    }

    flags_objs->out = NULL;

    return FLAGS_ERROR_SUCCESS;
}

enum FlagsError flags_objs_dtor (flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    if (flags_objs->out && fclose(flags_objs->out))
    {
        perror("Can't fclose out file");
        return FLAGS_ERROR_FAILURE;
    }

    return FLAGS_ERROR_SUCCESS;
}

enum FlagsError flags_processing(flags_objs_t* const flags_objs, 
                                 const int argc, char* const argv[])
{
    lassert(!is_invalid_ptr(flags_objs), "");
    lassert(!is_invalid_ptr(argv), "");
    lassert(argc, "");

    int getopt_rez = 0;
    while ((getopt_rez = getopt(argc, argv, "l:i:o:")) != -1)
    {
        switch (getopt_rez)
        {
            case 'l':
            {
                if (!strncpy(flags_objs->log_folder, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->log_folder");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }
            case 'i':
            {
                if (!strncpy(flags_objs->in_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->in_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }
            case 'o':
            {
                if (!strncpy(flags_objs->out_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->out_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            default:
            {
                fprintf(stderr, "Getopt error - d: %d, c: %c\n", getopt_rez, (char)getopt_rez);
                return FLAGS_ERROR_FAILURE;
            }
        }
    }
    
    if (!(flags_objs->out = fopen(flags_objs->out_filename, "wb")))
    {
        perror("Can't open out file");
        return FLAGS_ERROR_FAILURE;
    }

    return FLAGS_ERROR_SUCCESS;
}