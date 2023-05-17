#include "x64_elf.h"
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../lib/file.h"

const Elf64_Ehdr ELF_HEADER = {
        .e_ident = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 0}, // TODO Constify
        .e_type    = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,

        .e_entry   = 0x401000,

        .e_phoff    = sizeof(Elf64_Ehdr),          // Offset of programm header table. We took size of elf header
        .e_shoff    = 0,                           // Offset of segment header table. Not used

        .e_flags    = 0,                           // Extra flags: no flags
        .e_ehsize   = sizeof(Elf64_Ehdr),	       // Size of this header.

        .e_phentsize = sizeof(Elf64_Phdr),         // Size of Programm header table entry.
        .e_phnum     = 3,                          // Number of pheader entries. (system + stdlib + code + bss)

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
        .p_offset = 4096        , /* (bytes into file) */
        .p_vaddr  = 0x401000   , /* (virtual addr at runtime) */
        .p_paddr  = 0x401000   , /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = 0          , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr BSS_PHEADER = {
        .p_type   = 1          , /* [PT_LOAD] */
        .p_flags  = 0x6        , /* PF_R | PF_W */
        .p_offset = 0          , /* (bytes into file) */
        .p_vaddr  = 0x402000   , /* (virtual addr at runtime) */
        .p_paddr  = 0x402000   , /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = 104        , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};


const uint8_t code[] = {
        0xc6, 0x04, 0x25, 0x00, 0x20, 0x40, 0x00 ,0x53 ,0xc6 ,0x04 ,0x25 ,0x01 ,0x20 ,0x40 ,0x00 ,0x55,
        0xc6, 0x04, 0x25, 0x02, 0x20, 0x40, 0x00 ,0x53 ,0xc6 ,0x04 ,0x25 ,0x03 ,0x20 ,0x40 ,0x00 ,0x0a,
        0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01 ,0x00 ,0x00 ,0x00 ,0x48 ,0xbe ,0x00 ,0x20 ,0x40 ,0x00,
        0x00, 0x00, 0x00, 0x00, 0xba, 0x04, 0x00 ,0x00 ,0x00 ,0x0f ,0x05 ,0xb8 ,0x3c ,0x00 ,0x00 ,0x00,
        0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x05 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

result_t x64::save(code_t *self, const char *filename) {
    assert(self && filename && "Invalid pointers");
    FILE *binary = open_file_or_warn(filename, "wb");

    Elf64_Phdr code_pheader = CODE_PHEADER_TEMPLATE;
    code_pheader.p_filesz = sizeof(code); // TODO DEBUG changecd
    code_pheader.p_memsz  = sizeof(code); // TODO DEBUG change

    // Dump Headers
    // TODO Error handling
    fwrite(&ELF_HEADER, sizeof(ELF_HEADER), 1, binary);
    fwrite(&SYSTEM_PHEADER, sizeof(SYSTEM_PHEADER), 1, binary);
    fwrite(&code_pheader, sizeof(code_pheader), 1, binary);
    fwrite(&BSS_PHEADER, sizeof(BSS_PHEADER), 1, binary);
    fwrite(&BSS_PHEADER, sizeof(BSS_PHEADER), 1, binary); //FIXME Здесь нужна stdlib

    // TODO Write stdlib.

    uint size = 4096 - 4 * sizeof(BSS_PHEADER) - sizeof(ELF_HEADER);
    uint8_t *zero_buf = (uint8_t *) calloc(size, 1);

    fwrite(zero_buf, size, 1, binary);
    fwrite(&code, sizeof(code), 1, binary);

    fclose(binary);
    return result_t::OK;
}