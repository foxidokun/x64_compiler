#ifndef X64_TRANSLATOR_X64_H
#define X64_TRANSLATOR_X64_H

#include <stdint.h>
#include "../ir/ir.h"
#include "../lib/file.h"
#include "../lib/address_translator.h"


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

//----------------------------------------------------------------------------------------------------------------------

    result_t code_ctor(code_t *self);
    code_t  *code_new ();
    void code_dtor(code_t *self);
    void code_delete(code_t *self);

    result_t translate_from_ir(code_t *self, ir::code_t *ir_code);

    [[noreturn]]
    void execute(code_t *self);
}

#endif //X64_TRANSLATOR_X64_H
