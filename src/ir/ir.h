#ifndef X64_TRANSLATOR_IR_H
#define X64_TRANSLATOR_IR_H

#include <stdint.h>
#include <stdlib.h>

namespace ir {
    enum class instruction_type_t {
        PUSH = 0, // TODO remove 0
        POP,
        ADD,
        SUB,
        MUL,
        DIV,
        INC,
        DEC,
        SIN,
        COS,
        RET,
        HALT,
        INP,
        OUT,
        SQRT,
        CALL,
        JMP,
        JE,
        JNE,
        JBE,
        JB,
        JA,
        JAE,
        //...
    };

//----------------------------------------------------------------------------------------------------------------------

    struct instruction_t {
        instruction_type_t type;
        struct {
            unsigned char need_imm_arg: 1;
            unsigned char need_reg_arg: 1;
            unsigned char need_mem_arg: 1;
        };

        unsigned char reg_num;
        uint64_t imm_arg;

        size_t index;
        instruction_t *next;
    };

//----------------------------------------------------------------------------------------------------------------------

    struct code_t {
        instruction_t *instructions;
        size_t size;

        instruction_t *last_instruction;
    };

//----------------------------------------------------------------------------------------------------------------------

    code_t *code_new();
    void code_delete(code_t *self);

    void code_insert(code_t *self, instruction_t *instruction);
    ir::instruction_t *next_insruction(instruction_t *self);
}

#endif //X64_TRANSLATOR_IR_H
