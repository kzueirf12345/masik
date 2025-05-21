#ifndef MASIK_BACKEND_SRC_TRANSLATIN_FUNCS_ELF_HEADERS_H
#define MASIK_BACKEND_SRC_TRANSLATIN_FUNCS_ELF_HEADERS_H

#include "translation/funcs/elf/structs.h"
#include "translation/verification/verification.h"

enum TranslationError elf_headers_ctor(elf_translator_t* const translator, 
                                       elf_headers_t* const elf_headers);

enum TranslationError write_elf(const elf_translator_t* const translator, 
                                const elf_headers_t* const elf_headers,
                                FILE* out);

#endif /*MASIK_BACKEND_SRC_TRANSLATIN_FUNCS_ELF_HEADERS_H*/