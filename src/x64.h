#ifndef X64_TRANSLATOR_X64_H
#define X64_TRANSLATOR_X64_H

#include <stdint.h>
#include "ir.h"
#include "file.h"
#include "address_translator.h"


namespace x64 {
    struct code_t {
        uint8_t *exec_buf;
        size_t exec_buf_capacity;
        size_t exec_buf_size;

        uint8_t *ram_buf;
        size_t ram_buf_capacity;

        addr_transl_t *addr_transl;
        uint pass_index;
    };

    code_t *code_new();
    void code_delete(code_t *self);

    code_t *translate_from_ir(ir::code_t *ir_code);

    [[noreturn]]
    void execute(code_t *self);

    // https://wiki.osdev.org/X86-64_Instruction_Encoding
    struct instruction_t
    {
        struct
        {
            uint8_t require_REX    :1;
            uint8_t require_prefix :1;
            uint8_t require_ModRM  :1;
            uint8_t require_SIB    :1;
            uint8_t require_imm32  :1;
            uint8_t require_imm64  :1;
        };

        uint8_t REX;
        uint8_t prefix;
        uint8_t opcode;
        uint8_t ModRM;
        uint8_t SIB;

        uint32_t imm32;
        uint64_t imm64;
    };

}

#endif //X64_TRANSLATOR_X64_H
