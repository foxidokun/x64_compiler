#ifndef X64_TRANSLATOR_X64_GENERATORS_H
#define X64_TRANSLATOR_X64_GENERATORS_H

#include "x64.h"

namespace x64 {
    void emit_push_or_pop      (code_t *self, ir::instruction_t *ir_instruct);
    void emit_add_or_sub       (code_t *self, ir::instruction_t *ir_instruct);
    void emit_mull_or_div      (code_t *self, ir::instruction_t *ir_instruct);
    void emit_lib_func         (code_t *self, ir::instruction_t *ir_instruct);
    void emit_ret              (code_t *self);
    void emit_code_preparation (code_t *self);
    void emit_jmp_or_call      (code_t *self, ir::instruction_t *ir_instruct);
    void emit_cond_jmp         (code_t *self, ir::instruction_t *ir_instruct);
}

#endif //X64_TRANSLATOR_X64_GENERATORS_H
