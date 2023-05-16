#include "x64/x64.h"
#include "ir/ir.h"
#include "ir/ast_converter.h"
#include "lib/tree.h"
#include "lib/file.h"

const char BIN_NAME[] = "data/test.bin";

void print_ir(ir::code_t *code);

int main() {
    tree::node_t *ast = nullptr;
    const mmaped_file_t src = mmap_file_or_warn("data/quad.asm");

    const char *tree_section = (char *) src.data;
    while (*tree_section != '{') tree_section++;
    ast = tree::load_tree (tree_section);

    ir::code_t *ir_code = ir::from_ast(ast);

    print_ir(ir_code);
    x64::code_t *x64_code = x64::code_new();
    x64::translate_from_ir(x64_code, ir_code);
    execute(x64_code);
}


//TODO УБРАТЬ ЭТОТ КОД В РЕЛИЗЕ
const char INS_NAMES[][6] = {
        "PUSH",
        "POP",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "INC",
        "DEC",
        "SIN",
        "COS",
        "RET",
        "HALT",
        "INP",
        "OUT",
        "SQRT",
        "CALL",
        "JMP",
        "JE",
        "JNE",
        "JBE",
        "JB",
        "JA",
        "JAE"};

const char REGS[][10] = {"RAX", "RBX", "RCX", "RDX"};

void print_ir(ir::code_t *code) {
    ir::instruction_t *inst = code->instructions;

    while (inst) {
        printf("%d: %s ", inst->index, INS_NAMES[(int) inst->type]);
        if (inst->need_mem_arg) {
            printf("[] ");
        }

        if (inst->need_reg_arg) {
            printf("%s ", REGS[inst->reg_num]);
        }

        if (inst->need_imm_arg) {
            printf("%d", inst->imm_arg);
        }

        printf ("\n");

        inst = ir::next_insruction(inst);
    }
}