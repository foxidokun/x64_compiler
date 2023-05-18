#ifndef X64_TRANSLATOR_X64_ELF_H
#define X64_TRANSLATOR_X64_ELF_H

#include "../common.h"
#include "x64.h"

namespace x64 {
    enum BASE_ADDRESSES {
        CODE_BASE_ADDR   = 0x401000,
        STDLIB_BASE_ADDR = 0x403000,
        RAM_BASE_ADDR    = 0x404000,
    };

    enum class STDLIB_BINARY_OFFSETS {
        INPUT  = 0,
        OUTPUT = 0xAD,
        EXIT   = 0x1EF,
        SQRT   = 0x1FB
    };

    const int STDLIB_SIZE = 526;
    const int STDLIB_FILE_POS = 4096;
    const int CODE_FILE_POS   = 8192;

    const int RAMSIZE = 4096;

    const char STDLIB_FILENAME[] = "./src/asm_stdlib/stdlib.out";

    result_t save(code_t *self, const char *filename);
}

#endif //X64_TRANSLATOR_X64_ELF_H
