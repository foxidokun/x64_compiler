#ifndef X64_TRANSLATOR_IR_H
#define X64_TRANSLATOR_IR_H

#include <stdint.h>
#include <stdlib.h>

namespace ir {
    enum class instruction_type_t {
        PUSH,
        POP
        //...
    };

    struct instruction_t {
        instruction_type_t type;
        struct {
            unsigned char need_imm_arg: 1;
            unsigned char need_reg_arg: 1;
            unsigned char need_mem_arg: 1;
        };

        unsigned char reg_num;
        uint64_t imm_arg;
    };

    struct code_t {
        instruction_t *instructions;
        size_t size;
        size_t capacity;
    };

    code_t *code_new();
    void code_delete(code_t *self);

    void code_insert(code_t *self, instruction_t *instruction);
}

#endif //X64_TRANSLATOR_IR_H
