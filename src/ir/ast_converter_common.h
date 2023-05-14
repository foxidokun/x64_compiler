#ifndef X64_TRANSLATOR_AST_CONVERTER_COMMON_H
#define X64_TRANSLATOR_AST_CONVERTER_COMMON_H

#include "../lib/address_translator.h"
#include "../lib/tree.h"

namespace ir {
    struct vars_t {
        int *name_indexes;

        unsigned int size;
        unsigned int capacity;
    };

    struct converter_t {
        vars_t global_vars;
        vars_t local_vars;

        bool in_func;
        int global_frame_size_store;
        int frame_size;

        uint cur_label_index;

        addr_transl_t *indexed_label_transl;
        addr_transl_t *func_label_transl;
        uint pass_index;

        size_t current_instruction_index;
    };

// -------------------------------------------------------------------------------------------------
// From ast_converter.cpp
    result_t subtree_convert(converter_t *converter, tree::node_t * node, code_t * ir_code, bool result_used = true);
    void emit_instruction(converter_t *converter, code_t *ir_code, instruction_t *ir_instruct);
    uint get_label_index(converter_t *converter);
    void register_numeric_label (converter_t *converter, uint64_t label_num);
    void register_function_label(converter_t *converter, uint64_t func_num);
    void update_last_instruction_args(converter_t *converter, code_t *ir_code, instruction_t *instruction);

    // From ast_converter_generators.cpp
    void emit_code_begin(converter_t *converter, code_t *ir_code);
    void emit_code_end  (converter_t *converter, code_t *ir_code);

    result_t convert_op       (converter_t *converter, tree::node_t *node, code_t *ir_code);
    result_t convert_if       (converter_t *converter, tree::node_t *node, code_t *ir_code);
    result_t convert_while    (converter_t *converter, tree::node_t *node, code_t *ir_code);
    result_t convert_func_call(converter_t *converter, tree::node_t *node, code_t *ir_code);
    result_t convert_func_def (converter_t *converter, tree::node_t *node, code_t *ir_code);
}

#endif //X64_TRANSLATOR_AST_CONVERTER_COMMON_H
