#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"
#include "logger/liblogger.h"

enum PtrState is_invalid_ptr(const void* ptr)
{
    errno = 0;
    if (ptr == NULL)
    {
        return PTR_STATES_NULL;
    }

    char filename[] = "/tmp/chupapi_munyanya.XXXXXX";
    const int fd = mkstemp(filename);

    if (fd == -1) 
    {
        perror("Can't mkstemp file");
        return PTR_STATES_ERROR;
    }
    
    const ssize_t write_result = write(fd, ptr, 1);

    if (remove(filename))
    {
        perror("Can't remove temp file");
        return PTR_STATES_ERROR;
    }

    if (close(fd))
    {
        perror("Can't close temp file");
        return PTR_STATES_ERROR;
    }

    if (write_result == 1)
        return PTR_STATES_VALID;

    if (errno == EFAULT) 
    {
        errno = 0;
        return PTR_STATES_INVALID;
    }
    
    perror("Unpredictable errno state, after write into temp file");
    return PTR_STATES_ERROR;
}

int is_empty_file (FILE* file)
{
    if (is_invalid_ptr(file))
    {
        fprintf(stderr, "Is empty file nvalid\n");
        return -1;
    }

    int seek_temp = SEEK_CUR;

    if (fseek(file, 0, SEEK_END))
    {
        fprintf(stderr, "Can't fseek file\n");
        return -1;
    }

    const int res = ftell(file) > 2;

    if (fseek(file, 0, seek_temp))
    {
        fprintf(stderr, "Can't fseek file\n");
        return -1;
    }

    return res;
}

static int str_size_from_file_(size_t* const str_size, const int fd);

int str_from_file(const char* const filename, wchar_t** str, size_t* const str_size)
{
    lassert(!is_invalid_ptr(filename), "");
    lassert(!is_invalid_ptr(str), "");
    lassert(str_size, "");

    int fd = open(filename, O_RDWR);
    if (fd == -1)
    {
        perror("Can't fopen input file");
        return 1;
    }

    if (str_size_from_file_(str_size, fd))
    {
        fprintf(stderr, "Can't str_size_from_file_\n");
        close(fd);
        return 1;
    }

    char* temp_str = mmap(NULL, *str_size, PROT_READ, MAP_PRIVATE, fd, 0);

    *str = calloc(*str_size + 1, sizeof(**str));

    if (!*str)
    {
        perror("Can't calloc *str");
        close(fd);
        return 1;
    }

    if (mbstowcs(*str, temp_str, *str_size * sizeof(**str)) == (size_t)-1)
    {
        perror("Can't mbstowcs");
        close(fd);
        return 1;
    }

    if (*str == MAP_FAILED)
    {
        perror("Can't mmap");
        close(fd);
        return 1;
    }

    if (close(fd))
    {
        perror("Can't fclose input file");
        return 1;
    }
    IF_DEBUG(fd = -1;)

    return 0;
}

static int str_size_from_file_(size_t* const str_size, const int fd)
{
    lassert(str_size, "");
    lassert(fd != -1, "");

    struct stat fd_stat = {};

    if (fstat(fd, &fd_stat))
    {
        perror("Can't fstat");
        return 1;
    }

    *str_size = (size_t)fd_stat.st_size;

    return 0;
}

bool isnum(const wchar_t chr)
{
    return L'0' <= chr && chr <= L'9';
}