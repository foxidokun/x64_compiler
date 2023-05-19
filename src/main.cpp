#include "x64/x64.h"
#include "ir/ir.h"
#include "ir/ast_converter.h"
#include "lib/tree.h"
#include "lib/file.h"
#include "x64/x64_elf.h"

result_t load_and_compile(const char *ast_filename, const char *output_filename);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        log(ERROR, "Invalid number of parameters: expected 2");
        fprintf(stderr, "Usage: x64_compiler <input ast file> <output binary file>");
        return ERROR;
    }


    result_t res = load_and_compile(argv[1], argv[2]);

    if (res == result_t::OK) {
        return 0;
    }

    log(ERROR, "Program failed: see logs");
}

result_t load_and_compile(const char *ast_filename, const char *output_filename) {
    const mmaped_file_t src = mmap_file_or_warn(ast_filename);
    UNWRAP_NULLPTR( src.data );

    const char *tree_section = (char *) src.data;
    tree::tree_t ast = tree::load_tree (tree_section);
    UNWRAP_NULLPTR(ast.head_node);

    ir::code_t *ir_code = ir::code_new(); UNWRAP_NULLPTR(ir_code);
    UNWRAP_ERROR (ir::from_ast(ir_code, &ast));
    tree::dtor(&ast);

    x64::code_t *x64_code = x64::code_new(x64::output_t::BINARY);
    UNWRAP_ERROR (x64::translate_from_ir(x64_code, ir_code));
    ir::code_delete(ir_code);

    UNWRAP_ERROR (x64::save(x64_code, output_filename));
    x64::code_delete(x64_code);

    return result_t::OK;
}