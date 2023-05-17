#ifndef X64_TRANSLATOR_X64_ELF_H
#define X64_TRANSLATOR_X64_ELF_H

#include "../common.h"
#include "x64.h"

namespace x64 {
    const uint64_t BIN_RAM_ADDR = 0x404000;

    result_t save(code_t *self, const char *filename);
}

#endif //X64_TRANSLATOR_X64_ELF_H
