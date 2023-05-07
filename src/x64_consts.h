#ifndef X64_TRANSLATOR_X64_CONSTS_H
#define X64_TRANSLATOR_X64_CONSTS_H

namespace x64 {
    const char REG_NAMES[][4] = {"rax",  // 0b000
                                "rcx", // 0b001
                                "rdx", // 0b010
                                "rbx", // 0b011
                                "rsp", // 0b100
                                "rbp", // 0b101
                                "rsi", // 0b110
                                "rdi", // 0b111
    };

    enum x64_OPCODES {
        PUSH_r32     = 0x50,
        PUSH_i32     = 0x68,
        PUSH_m32     = 0xFF,
    };
}

#endif //X64_TRANSLATOR_X64_CONSTS_H
