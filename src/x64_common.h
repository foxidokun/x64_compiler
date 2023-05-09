#ifndef X64_TRANSLATOR_X64_COMMON_H
#define X64_TRANSLATOR_X64_COMMON_H

#include "x64.h"

namespace x64 {
    void emit_instruction(code_t *self, instruction_t *x64_instruct);
}

#endif //X64_TRANSLATOR_X64_COMMON_H
