#include "x64_elf.h"
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../lib/file.h"


const int STDLIB_SIZE = 536;
const int RAMSIZE     = 4096;

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
        .p_offset = 8192        , /* (bytes into file) */
        .p_vaddr  = x64::CODE_BASE_ADDR, /* (virtual addr at runtime) */
        .p_paddr  = x64::CODE_BASE_ADDR, /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = 0          , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr STDLIB_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_X,
        .p_offset = 4096,
        .p_vaddr  = x64::STDLIB_BASE_ADDR, /* (virtual addr at runtime) */
        .p_paddr  = x64::STDLIB_BASE_ADDR, /* (physical addr at runtime) */
        .p_filesz = STDLIB_SIZE, /* (bytes in file) */
        .p_memsz  = STDLIB_SIZE, /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};

const Elf64_Phdr BSS_PHEADER = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_R | PF_W,
        .p_offset = 0          , /* (bytes into file) */
        .p_vaddr  = x64::RAM_BASE_ADDR   , /* (virtual addr at runtime) */
        .p_paddr  = x64::RAM_BASE_ADDR   , /* (physical addr at runtime) */
        .p_filesz = 0          , /* (bytes in file) */
        .p_memsz  = RAMSIZE        , /* (bytes in mem at runtime) */
        .p_align  = 4096       , /* (min mem alignment in bytes) */
};


const uint8_t code[] = {
        0xc6, 0x04, 0x25, 0x00, 0x20, 0x40, 0x00 ,0x53 ,0xc6 ,0x04 ,0x25 ,0x01 ,0x20 ,0x40 ,0x00 ,0x55,
        0xc6, 0x04, 0x25, 0x02, 0x20, 0x40, 0x00 ,0x53 ,0xc6 ,0x04 ,0x25 ,0x03 ,0x20 ,0x40 ,0x00 ,0x0a,
        0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01 ,0x00 ,0x00 ,0x00 ,0x48 ,0xbe ,0x00 ,0x20 ,0x40 ,0x00,
        0x00, 0x00, 0x00, 0x00, 0xba, 0x04, 0x00 ,0x00 ,0x00 ,0x0f ,0x05 ,0xb8 ,0x3c ,0x00 ,0x00 ,0x00,
        0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x05 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

const uint8_t stdlib_code[STDLIB_SIZE] = {
        0x48, 0x83, 0xec, 0x58, 0xba, 0x07, 0x00, 0x00,
        0x00, 0xbf, 0x01, 0x00, 0x00, 0x00, 0x48, 0xb8, 0x49, 0x4e, 0x50, 0x55, 0x54, 0x3a, 0x20, 0x00,
        0x48, 0x8d, 0x74, 0x24, 0x08, 0x48, 0x89, 0x44, 0x24, 0x08, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x0f,
        0x05, 0xba, 0x3f, 0x00, 0x00, 0x00, 0x48, 0x8d, 0x74, 0x24, 0x10, 0x31, 0xff, 0xb8, 0x00, 0x00,
        0x00, 0x00, 0x0f, 0x05, 0x0f, 0xbe, 0x54, 0x24, 0x10, 0x80, 0xfa, 0x2d, 0x74, 0x4c, 0x31, 0xf6,
        0x31, 0xc9, 0x80, 0xfa, 0x0a, 0x74, 0x57, 0x48, 0x8d, 0x44, 0x24, 0x10, 0x48, 0x01, 0xc1, 0x31,
        0xc0, 0x83, 0xea, 0x30, 0x48, 0x8d, 0x04, 0x80, 0x48, 0x83, 0xc1, 0x01, 0x48, 0x63, 0xd2, 0x48,
        0x8d, 0x04, 0x42, 0x0f, 0xbe, 0x11, 0x80, 0xfa, 0x0a, 0x75, 0xe6, 0x40, 0x84, 0xf6, 0x74, 0x09,
        0x48, 0x6b, 0xc0, 0x9c, 0x48, 0x83, 0xc4, 0x58, 0xc3, 0x48, 0x8d, 0x04, 0x80, 0x48, 0x83, 0xc4,
        0x58, 0x48, 0x8d, 0x04, 0x80, 0x48, 0xc1, 0xe0, 0x02, 0xc3, 0x0f, 0xbe, 0x54, 0x24, 0x11, 0xbe,
        0x01, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x00, 0x80, 0xfa, 0x0a, 0x75, 0xa9, 0x31, 0xc0,
        0x48, 0x83, 0xc4, 0x58, 0xc3, 0x48, 0xb8, 0x4f, 0x55, 0x54, 0x50, 0x55, 0x54, 0x3a, 0x20, 0x53,
        0xba, 0x08, 0x00, 0x00, 0x00, 0x48, 0x89, 0xfb, 0xbf, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xec,
        0x60, 0x48, 0x8d, 0x74, 0x24, 0x07, 0x48, 0x89, 0x44, 0x24, 0x07, 0xc6, 0x44, 0x24, 0x0f, 0x00,
        0xb8, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x48, 0x89, 0xdf, 0x66, 0x0f, 0xef, 0xc0, 0x48, 0xb8,
        0xc3, 0xf5, 0x28, 0x5c, 0x8f, 0xc2, 0xf5, 0x28, 0x48, 0xf7, 0xdf, 0x0f, 0x29, 0x44, 0x24, 0x40,
        0x48, 0x0f, 0x48, 0xfb, 0xc6, 0x44, 0x24, 0x50, 0x0a, 0xc6, 0x44, 0x24, 0x4d, 0x2e, 0x48, 0x89,
        0xfa, 0x0f, 0x29, 0x44, 0x24, 0x10, 0x48, 0xc1, 0xea, 0x02, 0x0f, 0x29, 0x44, 0x24, 0x20, 0x48,
        0xf7, 0xe2, 0x0f, 0x29, 0x44, 0x24, 0x30, 0x48, 0x89, 0xd6, 0x48, 0xc1, 0xee, 0x02, 0x48, 0x8d,
        0x04, 0xb6, 0x48, 0x89, 0xf1, 0x48, 0x8d, 0x04, 0x80, 0x48, 0xc1, 0xe0, 0x02, 0x48, 0x29, 0xc7,
        0x48, 0x89, 0xfe, 0x48, 0xbf, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x48, 0x89, 0xf0,
        0x48, 0xf7, 0xe7, 0x48, 0xc1, 0xea, 0x03, 0x48, 0x8d, 0x04, 0x92, 0x83, 0xc2, 0x30, 0x48, 0x01,
        0xc0, 0x88, 0x54, 0x24, 0x4e, 0x48, 0x29, 0xc6, 0x83, 0xc6, 0x30, 0x40, 0x88, 0x74, 0x24, 0x4f,
        0x48, 0x85, 0xc9, 0x74, 0x76, 0x4c, 0x8d, 0x44, 0x24, 0x4c, 0x4c, 0x89, 0xc6, 0x48, 0x89, 0xc8,
        0x48, 0xf7, 0xe7, 0x48, 0xc1, 0xea, 0x03, 0x48, 0x8d, 0x04, 0x92, 0x48, 0x01, 0xc0, 0x48, 0x29,
        0xc1, 0x83, 0xc1, 0x30, 0x88, 0x0e, 0x48, 0x89, 0xd1, 0x48, 0x89, 0xf2, 0x48, 0x83, 0xee, 0x01,
        0x48, 0x85, 0xc9, 0x75, 0xd8, 0x41, 0x8d, 0x40, 0x04, 0x29, 0xd0, 0x8d, 0x50, 0x01, 0x48, 0x85,
        0xdb, 0x79, 0x14, 0xb9, 0x3f, 0x00, 0x00, 0x00, 0x29, 0xc1, 0x48, 0x63, 0xc1, 0xc6, 0x44, 0x04,
        0x10, 0x2d, 0x89, 0xd0, 0x83, 0xc2, 0x01, 0x48, 0x98, 0xb9, 0x40, 0x00, 0x00, 0x00, 0x48, 0x63,
        0xd2, 0xbf, 0x01, 0x00, 0x00, 0x00, 0x48, 0x29, 0xc1, 0x48, 0x8d, 0x74, 0x0c, 0x10, 0xb8, 0x01,
        0x00, 0x00, 0x00, 0x0f, 0x05, 0x48, 0x83, 0xc4, 0x60, 0x5b, 0xc3, 0xba, 0x04, 0x00, 0x00, 0x00,
        0xb8, 0x03, 0x00, 0x00, 0x00, 0xeb, 0xb7, 0xb8, 0x3c, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x05, 0xf2, 0x48, 0x0f, 0x2a, 0xcf, 0xf2, 0x0f, 0x51, 0xc9, 0xf2, 0x48, 0x0f, 0x2c,
        0xf9, 0x48, 0x6b, 0xc7, 0x0a, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
    fwrite(&STDLIB_PHEADER, sizeof(STDLIB_PHEADER), 1, binary);
    fwrite(&BSS_PHEADER, sizeof(BSS_PHEADER), 1, binary); //FIXME Здесь нужна stdlib

    // TODO Write stdlib.

//  Align code to 0x1000 bytes
    uint size = 4096 - 4 * sizeof(BSS_PHEADER) - sizeof(ELF_HEADER);
    uint8_t *zero_buf = (uint8_t *) calloc(4096, 1);
    fwrite(zero_buf, size, 1, binary);

    fwrite(stdlib_code, sizeof(stdlib_code), 1, binary);

    size = 4096 - sizeof(stdlib_code);
    fwrite(zero_buf, size, 1, binary);
    fwrite(self->exec_buf, self->exec_buf_size, 1, binary);

    free(zero_buf);

    fclose(binary);
    return result_t::OK;
}