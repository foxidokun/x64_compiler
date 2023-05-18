#ifndef X64_TRANSLATOR_X64_CONSTS_H
#define X64_TRANSLATOR_X64_CONSTS_H

namespace x64 {
    const char REG_NAMES[][4] = {"rax",  // 0b000
                                 "rcx",  // 0b001
                                 "rdx",  // 0b010
                                 "rbx",  // 0b011
                                 "rsp",  // 0b100
                                 "rbp",  // 0b101
                                 "rsi",  // 0b110
                                 "rdi",  // 0b111
                                 "r8",   // 0b1000
                                 "r9",   // 0b1001
                                 "r10",  // 0b1010
                                 "r11",  // 0b1011
                                 "r12",  // 0b1100
                                 "r13",  // 0b1101
                                 "r14",  // 0b1110
                                 "r15",  // 0b1111
    };

    enum REGS {
        REG_RAX  = 0b0000,
        REG_RCX  = 0b0001,
        REG_RDX  = 0b0010,
        REG_RBX  = 0b0011,
        REG_RSP  = 0b0100,
        REG_RBP  = 0b0101,
        REG_RSI  = 0b0110,
        REG_RDI  = 0b0111,
        REG_R8   = 0b1000,
        REG_R9   = 0b1001,
        REG_R10  = 0b1010,
        REG_R11  = 0b1011,
        REG_R12  = 0b1100,
        REG_R13  = 0b1101,
        REG_R14  = 0b1110,
        REG_R15  = 0b1111,
    };

    enum x64_OPCODES {
        PUSH_reg      = 0x50,
        PUSH_imm      = 0x68,
        PUSH_mem      = 0xFF,
        POP_reg       = 0x58,
        POP_mem       = 0x8F,
        ADD_mem_reg   = 0x01,
        SUB_mem_reg   = 0x29,
        DIVMUL_reg    = 0xF7,
        MOV_reg_imm   = 0x48,
        MOV_reg_imm64 = 0xC7,
        CALL_reg      = 0xFF,
        CMP_reg_reg   = 0x39,
        RET_none      = 0xC3,
        CQO_none      = 0x99,

        // Cond jumps prefix
        CONDJMP_imm_prefix = 0x0F,

        // Cond jumps opcodes
        JNGE_imm = 0x8c, // jl
        JNL_imm  = 0x8d, // jge
        JE_imm   = 0x84, // je
        JNE_imm  = 0x85, // jne
        JNG_imm  = 0x8e, // jle
        JG_imm   = 0x8f, // jg
    };

    // --- --- --- Some opcode consts --- --- ---
    const int EXTENDED_REG_MASK         = 0b1000;       // REX part of register
    const int LOWER_REG_BITS_MASK       = 0b0111;       // SIB part of register
    const int REX_BYTE_IF_NUM_REGS      = 0b01000001;   // REX mask if using numered register in instruction
    const int REX_BYTE_IF_64_BIT        = 0b01001000;

    const int IMM_MODRM_MODE_BIT        = 0b10000000;
    const int DOUBLE_REG_MODRM_MODE_BIT = 0b00000100;
    const int SINGLE_REG_MODRM_MODE_BIT = 0b00000000;
    const int ONLY_REG_MODRM_MODE_BIT   = 0b11000000;

    const int MODRM_MUL_REG_BITS        = 0b00101000;
    const int MODRM_DIV_REG_BITS        = 0b00111000;

    const int PUSH_MOD_REG_BITS         = 0b00110000;
    const int POP_MOD_REG_BITS          = 0b00000000;

    const int CALL_MOD_REG_BITS         = 0b00010000;
    const int JMP_MOD_REG_BITS          = 0b00100000;

    const int MODRM_RM_OFFSET           = 3;

    const int SIB_INDEX_OFFSET          = 3;
    const int SIB_BASE_OFFSET           = 0;

    const int DEBUG_SYSCALL_BYTE        = 0xCC;
}

#endif //X64_TRANSLATOR_X64_CONSTS_H
