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

    const uint64_t BIN_RAM_ADDR = 0x404000;

    result_t save(code_t *self, const char *filename);
}

#endif //X64_TRANSLATOR_X64_ELF_H
