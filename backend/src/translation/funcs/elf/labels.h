#ifndef MASIK_BACKEND_TRANSLATION_FUNCS_ELF_LABELS_H
#define MASIK_BACKEND_TRANSLATION_FUNCS_ELF_LABELS_H

#include "translation/funcs/elf/structs.h"
#include "translation/verification/verification.h"

enum TranslationError labels_val_ctor(labels_val_t* const val, const size_t label_addr);

enum TranslationError add_not_handle_addr(elf_translator_t* const translator, 
                                           label_t* const label_name, 
                                           const size_t insert_addr);

enum TranslationError add_label(elf_translator_t* const translator, label_t* const label_name);

enum TranslationError labels_processing(elf_translator_t* const translator);

#endif /*MASIK_BACKEND_TRANSLATION_FUNCS_ELF_LABELS_H*/