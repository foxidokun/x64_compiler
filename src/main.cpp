#include "x64/x64.h"
#include "ir/ir.h"
#include "ir/ast_converter.h"
#include "lib/tree.h"
#include "lib/file.h"
#include "x64/x64_elf.h"

const char BIN_NAME[] = "data/test.bin";

void print_ir(ir::code_t *code);

int main() {
    const mmaped_file_t src = mmap_file_or_warn("data/quad.asm");

    const char *tree_section = (char *) src.data;
    while (*tree_section != '{') tree_section++;
    tree::tree_t ast = tree::load_tree (tree_section);

    ir::code_t *ir_code = ir::code_new();
    ir::from_ast(ir_code, &ast);

    x64::code_t *x64_code = x64::code_new(x64::output_t::BINARY);
    x64::translate_from_ir(x64_code, ir_code);
    x64::save(x64_code, "/tmp/test.bin");

    tree::dtor(&ast);
    ir::code_delete(ir_code);
    x64::code_delete(x64_code);
}
