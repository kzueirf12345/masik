#include <string.h>
#include <getopt.h>

#include "flags.h"
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

    if (!strncpy(flags_objs->in_filename, "../assets/midle2_out.pyam", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->in_filename");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->splu_filename, "../assets/back_out.asm", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->splu_filename,");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->nasm_filename, "../assets/masik_nasm.asm", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->nasm_filename");
        return FLAGS_ERROR_SUCCESS;
    }

    if (!strncpy(flags_objs->elf_filename, "../masik_elf.out", FILENAME_MAX))
    {
        perror("Can't strncpy flags_objs->elf_filename");
        return FLAGS_ERROR_SUCCESS;
    }

    flags_objs->splu_out    = NULL;
    flags_objs->nasm_out    = NULL;
    flags_objs->elf_out     = NULL;

    return FLAGS_ERROR_SUCCESS;
}

enum FlagsError flags_objs_dtor (flags_objs_t* const flags_objs)
{
    lassert(!is_invalid_ptr(flags_objs), "");

    if (flags_objs->splu_out && fclose(flags_objs->splu_out))
    {
        perror("Can't fclose flags_objs->splu_out");
        return FLAGS_ERROR_FAILURE;
    }

    if (flags_objs->nasm_out && fclose(flags_objs->nasm_out))
    {
        perror("Can't fclose flags_objs->nasm_out");
        return FLAGS_ERROR_FAILURE;
    }

    if (flags_objs->elf_out && fclose(flags_objs->elf_out))
    {
        perror("Can't fclose flags_objs->elf_out");
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
    while ((getopt_rez = getopt(argc, argv, "l:i:s:a:e:")) != -1)
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
            case 's':
            {
                if (!strncpy(flags_objs->splu_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->splu_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'a':
            {
                if (!strncpy(flags_objs->nasm_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->nasm_filename");
                    return FLAGS_ERROR_FAILURE;
                }

                break;
            }

            case 'e':
            {
                if (!strncpy(flags_objs->elf_filename, optarg, FILENAME_MAX))
                {
                    perror("Can't strncpy flags_objs->elf_filename");
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

    if (!(flags_objs->splu_out = fopen(flags_objs->splu_filename, "wb")))
    {
        perror("Can't open splu_out file");
        return FLAGS_ERROR_FAILURE;
    }

    if (!(flags_objs->nasm_out = fopen(flags_objs->nasm_filename, "wb")))
    {
        perror("Can't open asm_out file");
        return FLAGS_ERROR_FAILURE;
    }

    if (!(flags_objs->elf_out = fopen(flags_objs->elf_filename, "wb")))
    {
        perror("Can't open elf_out file");
        return FLAGS_ERROR_FAILURE;
    }
    
    return FLAGS_ERROR_SUCCESS;
}