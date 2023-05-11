#include "x64/x64.h"
#include "ir.h"
#include "stackcpu.h"

const char BIN_NAME[] = "data/test.bin";

int main() {
    stackcpu::code_t *stackcpu_code = stackcpu::load(BIN_NAME);
    ir::code_t *ir_code = stackcpu::translate_to_ir(stackcpu_code);
    stackcpu::unload(stackcpu_code);

    x64::code_t *x64_code = x64::translate_from_ir(ir_code);
    x64::execute(x64_code);
}
