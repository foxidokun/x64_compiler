#ifndef X64_TRANSLATOR_STACKCPU_H
#define X64_TRANSLATOR_STACKCPU_H

#include <stdint.h>
#include <stdlib.h>
#include "ir.h"
#include "file.h"

namespace stackcpu {
    const int SUPPORTED_BINARY_VER = 7;
    const int SUPPORTED_HEADER_VER = 1;

    struct pre_header_t {
        uint64_t signature;
        unsigned char binary_version;
        unsigned char header_version;
    };

    struct header_v1_t {
        uint64_t hash;
        size_t code_size;
    };

    struct opcode_t {
        unsigned char opcode: 5;
        unsigned char m: 1;
        unsigned char r: 1;
        unsigned char i: 1;
    };

    #define CMD_DEF(name, number, ...) name = number,
    enum class opcode_type_t {
        #include "stackcpu_opcodes.h"
    };
    #undef CMD_DEF

    struct code_t {
        uint8_t *binary;
        size_t binary_size;

        mmaped_file_t mmaped_data;
    };

    code_t *load(const char *filename);
    void unload(code_t *self);

    ir::code_t *translate_to_ir(const code_t *self);
}

#endif //X64_TRANSLATOR_STACKCPU_H
