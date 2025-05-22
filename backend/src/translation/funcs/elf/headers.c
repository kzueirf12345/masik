#include "headers.h"

enum TranslationError elf_headers_ctor(elf_translator_t* const translator, elf_headers_t* const elf_headers)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elf_headers), "");

    const size_t text_size = stack_size(translator->text);
    const size_t text_align_size = text_size + ALIGN_ - text_size % ALIGN_;

    const char* shstrtab = 
        "\0"
        ".shstrtab\0"
        ".text"; 

    const size_t shstrtab_size = sizeof(
        "\0"
        ".shstrtab\0"
        ".text"
    );

    Elf64_Ehdr elf_header =
    {
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0},
        .e_type = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,
        .e_entry = ENTRY_ADDR_,
        .e_phoff = sizeof(Elf64_Ehdr),              
        .e_shoff = ALIGN_ + text_align_size,
        .e_flags = 0,
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum = 1,
        .e_shentsize = sizeof(Elf64_Shdr),
        .e_shnum = 3,
        .e_shstrndx = 1,
    }; 

    Elf64_Phdr elf_prog_header_text =
    {
        .p_type = PT_LOAD,
        .p_flags = PF_X | PF_R,
        .p_offset = ALIGN_,
        .p_vaddr = elf_header.e_entry,
        .p_paddr = elf_header.e_entry,
        .p_filesz = text_size,
        .p_memsz = text_size,
        .p_align = ALIGN_,
    };

    Elf64_Shdr section_headers[] = {
        // Null section
        {
            .sh_name = 0,
            .sh_type = SHT_NULL,
            .sh_flags = 0,
            .sh_addr = 0,
            .sh_offset = 0,
            .sh_size = 0,
            .sh_link = SHN_UNDEF,
            .sh_info = 0,
            .sh_addralign = 0,
            .sh_entsize = 0
        },
        
        // .shstrtab
        {
            .sh_name = 1,
            .sh_type = SHT_STRTAB,
            .sh_flags = SHF_STRINGS,
            .sh_addr = 0,
            .sh_offset = elf_header.e_shoff - shstrtab_size,
            .sh_size = shstrtab_size,
            .sh_link = 0,
            .sh_info = 0,
            .sh_addralign = 1,
            .sh_entsize = 0
        },
        
        // .text
        {
            .sh_name = 11,
            .sh_type = SHT_PROGBITS,
            .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
            .sh_addr = ENTRY_ADDR_,
            .sh_offset = ALIGN_,
            .sh_size = text_size,
            .sh_link = 0,
            .sh_info = 0,
            .sh_addralign = ALIGN_,
            .sh_entsize = 0
        },
    };

    elf_headers->ehdr = elf_header;
    elf_headers->phdr_text = elf_prog_header_text;

    elf_headers->shstrtab = shstrtab;
    elf_headers->shstrtab_size = shstrtab_size;

    elf_headers->shdr_zero = section_headers[0];
    elf_headers->shdr_shstrtab = section_headers[1];
    elf_headers->shdr_text = section_headers[2];

    return TRANSLATION_ERROR_SUCCESS;
}

enum TranslationError write_elf(const elf_translator_t* const translator, 
                                 const elf_headers_t* const elf_headers,
                                 FILE* out)
{
    lassert(!is_invalid_ptr(translator), "");
    lassert(!is_invalid_ptr(elf_headers), "");
    lassert(!is_invalid_ptr(out), "");

    if(fwrite(&elf_headers->ehdr, sizeof(elf_headers->ehdr), 1, out) != 1)
    {
        perror("Can't fwrite elf_header in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if(fwrite(&elf_headers->phdr_text, sizeof(elf_headers->phdr_text), 1, out) != 1)
    {
        perror("Can't fwrite elf_prog_header_text in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    uint8_t* align_zero_arr = calloc(ALIGN_, sizeof(*align_zero_arr));

    const size_t first_align_zero_cnt 
        = ALIGN_ - sizeof(elf_headers->ehdr) - sizeof(elf_headers->phdr_text);

    if(fwrite(align_zero_arr, sizeof(*align_zero_arr), first_align_zero_cnt, out) 
       != first_align_zero_cnt)
    {
        perror("Can't fwrite first align_zero_arr in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(stack_begin(translator->text), sizeof(uint8_t), stack_size(translator->text), out)
        < stack_size(translator->text))
    {
        perror("Can't fwrite .text code in output file");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    const size_t second_align_zero_cnt 
        = elf_headers->ehdr.e_shoff - elf_headers->shstrtab_size - stack_size(translator->text) - ALIGN_;

    if (fwrite(align_zero_arr, sizeof(*align_zero_arr), second_align_zero_cnt, out) 
        != second_align_zero_cnt)
    {
        perror("Can't fwrite second align_zero_arr in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    free(align_zero_arr);

    if (fwrite(elf_headers->shstrtab, sizeof(*elf_headers->shstrtab), elf_headers->shstrtab_size, out) 
       != elf_headers->shstrtab_size)
    {
        perror("Can't fwrite shstrtab in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(&elf_headers->shdr_zero, sizeof(elf_headers->shdr_zero), 1, out) != 1)
    {
        perror("Can't fwrite section_header zero in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(&elf_headers->shdr_shstrtab, sizeof(elf_headers->shdr_shstrtab), 1, out) != 1)
    {
        perror("Can't fwrite section_header shstrtab in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    if (fwrite(&elf_headers->shdr_text, sizeof(elf_headers->shdr_text), 1, out) != 1)
    {
        perror("Can't fwrite section_header .text in out");
        return TRANSLATION_ERROR_STANDARD_ERRNO;
    }

    return TRANSLATION_ERROR_SUCCESS;
}