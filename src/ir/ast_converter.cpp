#include <assert.h>
#include <cstdlib>
#include "../common.h"
#include "ast_converter.h"
#include "ast_converter_common.h"

// -------------------------------------------------------------------------------------------------
// Consts
// -------------------------------------------------------------------------------------------------

enum passes {
    PASS_INDEX_TO_CALC_OFFSETS =  0,
    PASS_INDEX_TO_WRITE,
    /// Total number of compile iterations
    TOTAL_PASS_COUNT
};

const int DEFAULT_VARS_CAPACITY       = 16;


// -------------------------------------------------------------------------------------------------
// Prototypes
// -------------------------------------------------------------------------------------------------

namespace ir {
    static converter_t *converter_new();
    static void converter_delete(converter_t *self);

    static result_t vars_ctor(vars_t *self);
    static void vars_dtor(vars_t *self);

    static void emit_instruction_calc_offset(converter_t *self, instruction_t *ir_instruct);
    static void emit_instruction_write(code_t *self, instruction_t *ir_instruct);

    static bool start_new_pass(converter_t *converter);
}

// -------------------------------------------------------------------------------------------------
// Public
// -------------------------------------------------------------------------------------------------

result_t ir::from_ast(ir::code_t *self, tree::node_t *node) {
    converter_t *converter = converter_new();
    if (!converter) {return result_t::ERROR;}

    bool need_another_pass = true;
    while (need_another_pass) {
        emit_code_begin(converter, self);

        result_t res = subtree_convert(converter, node, self, false);
        UNWRAP_ERROR(res);

        emit_code_end(converter, self);
        need_another_pass = start_new_pass(converter);
    }

    converter_delete(converter);
    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------
// Protected methods
// -------------------------------------------------------------------------------------------------

void ir::emit_instruction(converter_t *converter, code_t *ir_code, instruction_t *ir_instruct) {
    switch (converter->pass_index) {
        case PASS_INDEX_TO_CALC_OFFSETS:
            emit_instruction_calc_offset(converter, ir_instruct);
            break;

        case PASS_INDEX_TO_WRITE:
            emit_instruction_write(ir_code, ir_instruct);
            break;

        default:
            assert(0 && "Unexpected pass_index");
            break;
    }
}

// -------------------------------------------------------------------------------------------------

uint ir::get_label_index(converter_t *converter) {
    assert (converter != nullptr && "invalid pointer");
    return converter->cur_label_index++;
}

// -------------------------------------------------------------------------------------------------

void ir::register_numeric_label(converter_t *converter, uint64_t label_num) {
    if (converter->pass_index == PASS_INDEX_TO_CALC_OFFSETS) {
        addr_transl_insert(converter->indexed_label_transl, label_num, converter->current_instruction_index);
    }
}

// -------------------------------------------------------------------------------------------------

void ir::register_function_label(converter_t *converter, uint64_t func_num) {
    if (converter->pass_index == PASS_INDEX_TO_CALC_OFFSETS) {
        addr_transl_insert(converter->func_label_transl, func_num, converter->current_instruction_index);
    }
}

// -------------------------------------------------------------------------------------------------

void ir::update_last_instruction_args(converter_t *converter, code_t *ir_code, instruction_t *instruction) {
    if (converter->pass_index == PASS_INDEX_TO_WRITE) {
        ir_code->last_instruction->need_reg_arg = instruction->need_reg_arg;
        ir_code->last_instruction->need_imm_arg = instruction->need_imm_arg;
        ir_code->last_instruction->need_mem_arg = instruction->need_mem_arg;
        ir_code->last_instruction->reg_num      = instruction->reg_num;
        ir_code->last_instruction->imm_arg      = instruction->imm_arg;
    }
}

// -------------------------------------------------------------------------------------------------
// Static
// -------------------------------------------------------------------------------------------------

static ir::converter_t *ir::converter_new() {
    converter_t *self = (converter_t *) calloc(1, sizeof (converter_t));
    if (!self) { return nullptr; }

    vars_ctor(&self->global_vars);
    vars_ctor(&self->local_vars);

    self->cur_label_index         = 0;
    self->frame_size              = 0;
    self->global_frame_size_store = 0;
    self->pass_index              = 0;

    self->indexed_label_transl = addr_transl_new();
    self->func_label_transl    = addr_transl_new();

    return self;
}

// -------------------------------------------------------------------------------------------------

static void ir::converter_delete(converter_t *self) {
    if (self) {
        vars_dtor(&self->global_vars);
        vars_dtor(&self->local_vars);
        addr_transl_delete(self->indexed_label_transl);
        addr_transl_delete(self->func_label_transl);

        free(self);
    }
}

// -------------------------------------------------------------------------------------------------

static result_t ir::vars_ctor(vars_t *self) {
    assert(self);

    self->name_indexes = (int *) calloc (DEFAULT_VARS_CAPACITY, sizeof (int));
    if (!self->name_indexes) {return result_t::ERROR; }

    self->size     = 0;
    self->capacity = DEFAULT_VARS_CAPACITY;

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

static void ir::vars_dtor(vars_t *self) {
    assert (self != nullptr && "invalis pointer");

    free (self->name_indexes);
#ifndef NDEBUG
    self->size     = 0;
    self->capacity = 0;
#endif
}

// -------------------------------------------------------------------------------------------------

static void ir::emit_instruction_calc_offset(converter_t *converter, instruction_t *ir_instruct) {
    converter->current_instruction_index++;
}

static void ir::emit_instruction_write(code_t *self, instruction_t *ir_instruct) {
    code_insert(self, ir_instruct);
}

// -------------------------------------------------------------------------------------------------

static bool ir::start_new_pass(converter_t *converter) {
    if (converter->pass_index + 1 < TOTAL_PASS_COUNT) {
        converter->pass_index++;
        converter->cur_label_index = 0;
        converter->current_instruction_index = 0;

        converter->frame_size = 0;
        converter->global_frame_size_store = 0;

        return true;
    }

    return false;
}
