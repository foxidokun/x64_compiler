#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "log.h"
#include "stackcpu.h"

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

namespace stackcpu {
    static bool has_valid_header(const uint8_t *binary);

    static void insert_push_or_pop(ir::code_t *ir_code, const uint8_t **binary_ptr);
    static void insert_opcode_without_args(ir::code_t *ir_code, const uint8_t **binary_ptr);

    static void populate_args(ir::instruction_t *instruct, const uint8_t **binary_ptr);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

stackcpu::code_t *stackcpu::load(const char *filename) {
    mmaped_file_t mmaped_file = mmap_file_or_warn(filename);
    if (!mmaped_file.data) { return nullptr; }

    if (!has_valid_header(mmaped_file.data)) {
        mmap_close(mmaped_file);
        return nullptr;
    }

    header_v1_t *header = (header_v1_t *) (mmaped_file.data + sizeof(pre_header_t));

    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    self->binary = mmaped_file.data + sizeof(pre_header_t) + sizeof(header_v1_t);
    self->binary_size = header->code_size;
    self->mmaped_data = mmaped_file;

    log(INFO, "Loaded stackcpu binary");

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void stackcpu::unload(code_t *self) {
    if (self) {
        self->binary = nullptr;
        mmap_close(self->mmaped_data);
    }
}

//----------------------------------------------------------------------------------------------------------------------

ir::code_t *stackcpu::translate_to_ir(const stackcpu::code_t *self) {
    ir::code_t *ir_code = ir::code_new();
    const uint8_t *binary = self->binary;

    int opcode_size = 0;

    while (binary - self->binary < self->binary_size) {
        opcode_t opcode = *(opcode_t *) binary;

        switch ((opcode_type_t) opcode.opcode) {
            case opcode_type_t::push:
            case opcode_type_t::pop:
                log(INFO, "Translating PUSH/POP with m: %d, r: %d, i: %d", opcode.m, opcode.r, opcode.i);
                insert_push_or_pop(ir_code, &binary);
                break;

            case opcode_type_t::add:
            case opcode_type_t::sub:
            case opcode_type_t::mul:
            case opcode_type_t::div:
            case opcode_type_t::inc:
            case opcode_type_t::dec:
            case opcode_type_t::ret:
            case opcode_type_t::halt:
            case opcode_type_t::inp:
            case opcode_type_t::out:
            case opcode_type_t::sqrt:
                log (INFO, "Translating opcode without args");
                insert_opcode_without_args(ir_code, &binary);
                break;

            default:
                log(WARN, "Unsupported opcode %d, skipping...", opcode.opcode);
                opcode_size = 1;
                if (opcode.r) {opcode_size += 1;}
                if (opcode.i) {opcode_size += 4;}
                binary += opcode_size;
                break;
        }
    }

    return ir_code;
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static bool stackcpu::has_valid_header(const uint8_t *binary) {
    pre_header_t *pre_header = (pre_header_t *) binary;
    if (pre_header->header_version != SUPPORTED_HEADER_VER) {
        fprintf(stderr, "Unsupported header version: expected %d, got %d\n",
                SUPPORTED_HEADER_VER, pre_header->header_version);
        return false;
    }

    if (pre_header->binary_version != SUPPORTED_BINARY_VER) {
        fprintf(stderr, "Unsupported binary version: expected %d, got %d\n",
                SUPPORTED_BINARY_VER, pre_header->binary_version);
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void stackcpu::insert_push_or_pop(ir::code_t *ir_code, const uint8_t **binary_ptr) {
    const uint8_t *binary = *binary_ptr;
    opcode_t opcode = *(opcode_t *) binary;
    assert((opcode_type_t) opcode.opcode == opcode_type_t::push ||
           (opcode_type_t) opcode.opcode == opcode_type_t::pop);

    ir::instruction_t instruct = {};
    if ((opcode_type_t) opcode.opcode == opcode_type_t::push) {
        instruct.type = ir::instruction_type_t::PUSH;
    } else {
        instruct.type = ir::instruction_type_t::POP;
    }
    populate_args(&instruct, &binary);

    ir::code_insert(ir_code, &instruct);
    *binary_ptr = binary;
}

//----------------------------------------------------------------------------------------------------------------------

#define TRANSLATE_TYPE(stack_type, ir_type)                              \
    case opcode_type_t::stack_type:                                      \
        log (INFO, "opcode type: " #ir_type " (stack " #stack_type ")"); \
        instruct.type = ir::instruction_type_t::ir_type;                 \
        break;

static void stackcpu::insert_opcode_without_args(ir::code_t *ir_code, const uint8_t **binary_ptr) {
    const uint8_t *binary = *binary_ptr;
    opcode_t opcode = *(opcode_t *) binary;
    binary++;

    ir::instruction_t instruct = {};

    switch ((opcode_type_t) opcode.opcode) {
        TRANSLATE_TYPE(add,  ADD)
        TRANSLATE_TYPE(sub,  SUB)
        TRANSLATE_TYPE(inc,  INC)
        TRANSLATE_TYPE(dec,  DEC)
        TRANSLATE_TYPE(mul,  MUL)
        TRANSLATE_TYPE(div,  DIV)
        TRANSLATE_TYPE(ret,  RET)
        TRANSLATE_TYPE(halt, HALT)
        TRANSLATE_TYPE(inp,  INP)
        TRANSLATE_TYPE(out,  OUT)
        TRANSLATE_TYPE(sqrt, SQRT)

        default:
            assert(0 && "Unexpected opcode type");
    }

    ir::code_insert(ir_code, &instruct);
    *binary_ptr = binary;
}

#undef TRANSLATE_TYPE

//----------------------------------------------------------------------------------------------------------------------

static void stackcpu::populate_args(ir::instruction_t *instruct, const uint8_t **binary_ptr) {
    log(DEBUG, "Extracting args...");

    const uint8_t *binary = *binary_ptr;
    opcode_t opcode = *(opcode_t *) binary;
    binary++;

    if (opcode.i) {
        instruct->need_imm_arg = true;
        instruct->imm_arg = *(int *) binary;
        binary += sizeof(int);

        log(DEBUG, "\tFound imm arg = %d", instruct->imm_arg);
    }

    if (opcode.r) {
        instruct->need_reg_arg = true;
        instruct->reg_num = *(unsigned char*) binary;
        binary += sizeof(unsigned char);

        log(DEBUG, "\tFound reg arg = %d", instruct->reg_num);
    }

    if (opcode.m) {
        instruct->need_mem_arg = true;
        log(DEBUG, "\tFound mem bit");
    }

    *binary_ptr = binary;
}
