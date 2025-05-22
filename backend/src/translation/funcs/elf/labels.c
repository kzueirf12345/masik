#include "labels.h"
#include "hash_table/libhash_table.h"
#include "map_utils.h"

#define STACK_ERROR_HANDLE_(call_func, ...)                                                         \
    do {                                                                                            \
        const enum StackError stack_error_handler = call_func;                                      \
        if (stack_error_handler)                                                                    \
        {                                                                                           \
            fprintf(stderr, "Can't " #call_func". Stack error: %s\n",                               \
                            stack_strerror(stack_error_handler));                                   \
            __VA_ARGS__                                                                             \
            return TRANSLATION_ERROR_STACK;                                                         \
        }                                                                                           \
    } while(0)

enum TranslationError labels_val_ctor(labels_val_t* const val, const size_t label_addr)
{
    lassert(!is_invalid_ptr(val), "");

    val->label_addr = label_addr;
    STACK_ERROR_HANDLE_(STACK_CTOR(&val->insert_addrs, sizeof(size_t), 1));

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError add_not_handle_addr(elf_translator_t* const translator, 
                                                  label_t* const label_name, 
                                                  const size_t insert_addr)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(label_name), "");

    labels_val_t* const val = smash_map_get_val(&translator->labels_map, label_name);

    if (!val)
    {
        labels_val_t new_val = {};
        TRANSLATION_ERROR_HANDLE(labels_val_ctor(&new_val, 0));

        STACK_ERROR_HANDLE_(stack_push(&new_val.insert_addrs, &insert_addr));

        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->labels_map, 
                (smash_map_elem_t){.key = label_name, .val = &new_val}
            )
        );

        STACK_ERROR_HANDLE_(stack_push(&translator->labels_stack, label_name));
    }
    else
    {
        STACK_ERROR_HANDLE_(stack_push(&val->insert_addrs, &insert_addr));
    }

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError add_label(elf_translator_t* const translator, label_t* const label_name)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(label_name), "");
    
    labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, label_name);

    if (!labels_val)
    {
        labels_val_t new_val = {};
        TRANSLATION_ERROR_HANDLE(labels_val_ctor(&new_val, 0));

        new_val.label_addr = translator->cur_addr;
        
        SMASH_MAP_ERROR_HANDLE_(
            smash_map_insert(
                &translator->labels_map, 
                (smash_map_elem_t){.key = label_name, .val = &new_val}
            )
        );

        STACK_ERROR_HANDLE_(stack_push(&translator->labels_stack, label_name));
    }
    else
    {
        labels_val->label_addr = translator->cur_addr;
    }

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError labels_processing(elf_translator_t* const translator)
{
    lassert(!is_invalid_ptr(translator), "");

    for (size_t label_ind = 0; label_ind < stack_size(translator->labels_stack); ++label_ind)
    {
        label_t* label_key = stack_get(translator->labels_stack, label_ind);
        
        labels_val_t* labels_val = smash_map_get_val(&translator->labels_map, label_key);

        for (size_t insert_addr_ind = 0; insert_addr_ind < stack_size(labels_val->insert_addrs); ++insert_addr_ind)
        {
            size_t* insert_addr = stack_get(labels_val->insert_addrs, insert_addr_ind);
            
            // fprintf(stderr, RED_TEXT("labels_name: %s\n"), label_key->name);
            // fprintf(stderr, RED_TEXT("insert_addr: %x\n"), *insert_addr);
            // fprintf(stderr, RED_TEXT("label_addr: %x\n"), labels_val->label_addr);
            // fprintf(stderr, RED_TEXT("rel_addr: %x\n"), labels_val->label_addr - *insert_addr - 4);

            uint8_t* insert_place = stack_get(translator->text, *insert_addr - ENTRY_ADDR_); 

            size_t insert_num = (size_t)insert_place[0] 
                             + ((size_t)insert_place[1] << 8) 
                             + ((size_t)insert_place[2] << 16) 
                             + ((size_t)insert_place[3] << 24);

            // fprintf(stderr, "insert_num: %x\n", insert_num);

            size_t rel_addr = labels_val->label_addr - *insert_addr - 4 + insert_num;

            if (!memcpy(insert_place, (uint8_t*)&rel_addr, sizeof(uint32_t)))
            {
                perror("Can't memcpy label_addr in insert_addr");
                return TRANSLATION_ERROR_SUCCESS;
            }
        }
    }

    return TRANSLATION_ERROR_SUCCESS;
}