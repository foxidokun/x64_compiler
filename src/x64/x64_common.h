#ifndef X64_TRANSLATOR_X64_COMMON_H
#define X64_TRANSLATOR_X64_COMMON_H

#include "x64.h"

namespace x64 {
    /// https://wiki.osdev.org/X86-64_Instruction_Encoding
    struct instruction_t
    {
        struct // optional fields flags
        {
            bool require_REX    :1;
            bool require_prefix :1;
            bool require_ModRM  :1;
            bool require_SIB    :1;
            bool require_imm32  :1;
            bool require_imm64  :1;
        };

        uint8_t REX;
        uint8_t prefix;
        uint8_t opcode;
        uint8_t ModRM;
        uint8_t SIB;

        uint32_t imm32;
        uint64_t imm64;
    };

    void emit_instruction(code_t *self, instruction_t *x64_instruct);
}

#endif //X64_TRANSLATOR_X64_COMMON_H
