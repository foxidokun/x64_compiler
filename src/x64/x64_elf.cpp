#include "x64_elf.h"
#include <elf.h>

// -------------------------------------------------------------------------------------------------

const Elf64_Ehdr ELF_HEADER = {
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Magic signature
                    ELFCLASS64,                                     // 64-bit system
                    ELFDATA2LSB,                                    // LittleEndian / BigEndian
                    EV_CURRENT,                                     // Version = Current
                    ELFOSABI_NONE,                                  // Non specified system
                    0
                    },
        .e_type    = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,

        .e_entry   = 0x401000,

        .e_phoff    = sizeof(Elf64_Ehdr),          // Offset of programm header table. We took size of elf header
        .e_shoff    = 0,                           // Offset of segment header table. Not used

        .e_flags    = 0,                           // Extra flags: no flags
        .e_ehsize   = sizeof(Elf64_Ehdr),	       // Size of this header.

        .e_phentsize = sizeof(Elf64_Phdr),         // Size of Programm header table entry.
        .e_phnum     = 4,                          // Number of pheader entries. (system + stdlib + code + bss)

        .e_shentsize = sizeof(Elf64_Shdr),         // Size of Segment header entry.
        .e_shnum     = 0,                          // Number of segments in programm.
        .e_shstrndx  = 0,                          // Index of string table. (Explained in further parts).
};

const Elf64_Phdr SYSTEM_PHEADER = {
        .p_type   = 1          , /* [PT_LOAD] */
        .p_flags  = 0x4        , /* PF_R */
        .p_offset = 0          , /* (bytes into file) */
        .p_vaddr  = 0x400000   , /* (virtual addr at runtime) */
        .p_paddr  = 0x400000   , /* (physical addr at runtime) */
        .p_filesz = 288        , /* (bytes in file) */
        .p_memsz  = 288        , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr CODE_PHEADER_TEMPLATE = {
        .p_type   = 1          , /* [PT_LOAD] */
        .p_flags  = 0x5        , /* PF_R | PF_X */
        .p_offset = x64::CODE_FILE_POS, /* (bytes into file) */
        .p_vaddr  = x64::CODE_BASE_ADDR, /* (virtual addr at runtime) */
        .p_paddr  = x64::CODE_BASE_ADDR, /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = 0          , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr STDLIB_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_X,
        .p_offset = x64::STDLIB_FILE_POS,
        .p_vaddr  = x64::STDLIB_BASE_ADDR, /* (virtual addr at runtime) */
        .p_paddr  = x64::STDLIB_BASE_ADDR, /* (physical addr at runtime) */
        .p_filesz = x64::STDLIB_SIZE, /* (bytes in file) */
        .p_memsz  = x64::STDLIB_SIZE, /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr BSS_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_W,
        .p_offset = 0          , /* (bytes into file) */
        .p_vaddr  = x64::RAM_BASE_ADDR   , /* (virtual addr at runtime) */
        .p_paddr  = x64::RAM_BASE_ADDR   , /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = x64::RAMSIZE, /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

// -------------------------------------------------------------------------------------------------
// Macros
// -------------------------------------------------------------------------------------------------

#define UNWRAP_WRITE(expr) if ((expr) != 1) { return result_t::ERROR; }

// -------------------------------------------------------------------------------------------------

static result_t load_stdlib(FILE *output, const char *filename);

// -------------------------------------------------------------------------------------------------


result_t x64::save(code_t *self, const char *filename) {
    assert(self && filename && "Invalid pointers");
    FILE *binary = open_file_or_warn(filename, "wb");
    UNWRAP_NULLPTR(binary);

    Elf64_Phdr code_pheader = CODE_PHEADER_TEMPLATE;
    code_pheader.p_filesz = self->exec_buf_size;
    code_pheader.p_memsz  = self->exec_buf_size;

    // Dump Headers
    UNWRAP_WRITE (fwrite(&ELF_HEADER, sizeof(ELF_HEADER), 1, binary));
    UNWRAP_WRITE (fwrite(&SYSTEM_PHEADER, sizeof(SYSTEM_PHEADER), 1, binary));
    UNWRAP_WRITE (fwrite(&code_pheader, sizeof(code_pheader), 1, binary));
    UNWRAP_WRITE (fwrite(&STDLIB_PHEADER, sizeof(STDLIB_PHEADER), 1, binary));
    UNWRAP_WRITE (fwrite(&BSS_PHEADER, sizeof(BSS_PHEADER), 1, binary));

    // Write stdlib
    fseek(binary, STDLIB_FILE_POS, SEEK_SET);
    load_stdlib(binary, STDLIB_FILENAME);

    // Write code
    fseek(binary, CODE_FILE_POS, SEEK_SET);
    UNWRAP_WRITE (fwrite(self->exec_buf, self->exec_buf_size, 1, binary));

    fclose(binary);
    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------
// Static
// -------------------------------------------------------------------------------------------------

static result_t load_stdlib(FILE *output, const char *filename) {
    mmaped_file_t stdlib = mmap_file_or_warn(filename);
    UNWRAP_NULLPTR (stdlib.data);

    Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *) stdlib.data;
    assert(elf_hdr->e_phnum == 2 && "can't load with more then just 1 text segment"); // system + text only

    // Skip elf header and system pheader
    Elf64_Phdr *code_phdr = (Elf64_Phdr *) (stdlib.data + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr));
    assert(code_phdr->p_flags == PF_R | PF_X && "Unexpected: Not code segment");
    assert(code_phdr->p_filesz == x64::STDLIB_SIZE && "STDLIB changed");

    UNWRAP_WRITE (fwrite(stdlib.data + code_phdr->p_offset, code_phdr->p_filesz, 1, output));

    mmap_close(stdlib);
    return result_t::OK;
}