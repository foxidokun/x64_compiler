#include <assert.h>
#include <sys/mman.h>
#include "../common.h"
#include "x64_common.h"
#include "x64_generators.h"
#include "x64_consts.h"
#include "x64_elf.h"
#include "x64.h"

//----------------------------------------------------------------------------------------------------------------------

const int PAGE_SIZE                   = 4096;  // Standart memory page size
const int EXEC_BUF_THRESHOLD          = 15;    // maximum 15 bytes per x64 instruction

enum passes {
    PASS_INDEX_TO_CALC_OFFSETS =  0,
    PASS_INDEX_TO_WRITE,
    /// Total number of compile iterations
    TOTAL_PASS_COUNT
};

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

// Internal methods
namespace x64 {
    static void encode_one_ir_instruction(code_t *self, ir::instruction_t *ir_instruct);

    static void emit_instruction_calc_offset(code_t *self, instruction_t *x64_instruct);
    static void emit_instruction_write      (code_t *self, instruction_t *x64_instruct);

    static void resize_if_needed(code_t *self);
    static void start_new_pass  (code_t *self);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

x64::code_t *x64::code_new(output_t output) {
    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    if (!self) { return nullptr;}

    result_t ctor_res = code_ctor(self, output);
    if (ctor_res == result_t::ERROR) {
        free(self);
        return nullptr;
    }

    return self;
}

result_t x64::code_ctor(code_t *self, output_t output) {
    self->exec_buf = (uint8_t *) mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_WRITE,
                                                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (self->exec_buf == MAP_FAILED) { return result_t::ERROR;}

    if (output == output_t::JIT) {
        self->ram_buf = (uint8_t *) mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (self->ram_buf == MAP_FAILED) { return result_t::ERROR; }
    }

    self->exec_buf_capacity = PAGE_SIZE;
    self->ram_buf_capacity  = PAGE_SIZE;

    self->addr_transl = addr_transl_new();
    self->pass_index = 0;
    self->output_type = output;

    return result_t::OK;
}

//----------------------------------------------------------------------------------------------------------------------

void x64::code_delete(code_t *self) {
    if (self) {
        code_dtor(self);
    }
    free(self);
}

void x64::code_dtor(code_t *self) {
    munmap(self->exec_buf, self->exec_buf_capacity);
    munmap(self->ram_buf, self->ram_buf_capacity);
    addr_transl_delete(self->addr_transl);
}

//----------------------------------------------------------------------------------------------------------------------

result_t x64::translate_from_ir(x64::code_t *self, ir::code_t *ir_code) {
    while (self->pass_index < TOTAL_PASS_COUNT) {
        ir::instruction_t *ir_instruct = ir_code->instructions;

        emit_code_preparation(self);

        while (ir_instruct) {
            if (self->pass_index == PASS_INDEX_TO_CALC_OFFSETS) {
                result_t res = addr_transl_remember_old_addr(self->addr_transl, ir_instruct->index);
                UNWRAP_ERROR(res);
            }

            encode_one_ir_instruction(self, ir_instruct);

            ir_instruct = ir::next_insruction(ir_instruct);
        }

        start_new_pass(self);
    }

    return result_t::OK;
}

//----------------------------------------------------------------------------------------------------------------------

[[noreturn]]
void x64::execute(code_t *self) {
    asm ("mov %1, %%r8\n"
         "jmp %0\n": :"r" (self->exec_buf), "r" (self->ram_buf) : "r8");
}

//----------------------------------------------------------------------------------------------------------------------
// Generators
//----------------------------------------------------------------------------------------------------------------------

void x64::emit_instruction(code_t *self, instruction_t *x64_instruct) {
    switch (self->pass_index) {
        case PASS_INDEX_TO_CALC_OFFSETS:
            emit_instruction_calc_offset(self, x64_instruct);
            break;

        case PASS_INDEX_TO_WRITE:
            emit_instruction_write(self, x64_instruct);
            break;

        default:
            assert(0 && "Unexpected pass_index");
            break;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_instruction_calc_offset(code_t *self, instruction_t *x64_instruct) {
    uint command_size = 1; // Opcode
    if (x64_instruct->require_REX)    { command_size += 1; }
    if (x64_instruct->require_prefix) { command_size += 1; }
    if (x64_instruct->require_ModRM)  { command_size += 1; }
    if (x64_instruct->require_SIB)    { command_size += 1; }
    if (x64_instruct->require_imm32)  { command_size += sizeof(uint32_t); }
    if (x64_instruct->require_imm64)  { command_size += sizeof(uint64_t); }

    uint64_t base_addr = (self->output_type == output_t::JIT) ? (uint64_t) self->exec_buf : x64::CODE_BASE_ADDR;

    addr_transl_insert_if_remembered_addr(self->addr_transl,  base_addr + self->exec_buf_size);

    self->exec_buf_size += command_size;
}

//----------------------------------------------------------------------------------------------------------------------

#define EMIT_OPTIONAL_FIELD(fieldname, type)                                                                            \
    if (x64_instruct->require_##fieldname) {                                                                            \
        log (INFO, "\t " #fieldname " field: DEC=%d HEX=0x%x", x64_instruct->fieldname, x64_instruct->fieldname);       \
        *((type *) (self->exec_buf + self->exec_buf_size)) = x64_instruct->fieldname;                                   \
        self->exec_buf_size += sizeof(type);                                                                            \
    }

static void x64::emit_instruction_write(code_t *self, instruction_t *x64_instruct) {
    log (INFO, "Emitting instruction...");

    EMIT_OPTIONAL_FIELD(REX,    uint8_t)
    EMIT_OPTIONAL_FIELD(prefix, uint8_t)

    log(INFO, "\tOpcode: %x", x64_instruct->opcode);
    self->exec_buf[self->exec_buf_size++] = x64_instruct->opcode;

    EMIT_OPTIONAL_FIELD(ModRM, uint8_t)
    EMIT_OPTIONAL_FIELD(SIB,   uint8_t)
    EMIT_OPTIONAL_FIELD(imm32, uint32_t)
    EMIT_OPTIONAL_FIELD(imm64, uint64_t)

    resize_if_needed(self);
}

#undef EMIT_OPTIONAL_FIELD

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static void x64::encode_one_ir_instruction(code_t *self, ir::instruction_t *ir_instruct) {
    switch (ir_instruct->type) {
        case ir::instruction_type_t::PUSH:
        case ir::instruction_type_t::POP:
            emit_push_or_pop(self, ir_instruct);
            break;

        case ir::instruction_type_t::ADD:
        case ir::instruction_type_t::SUB:
            emit_add_or_sub(self, ir_instruct);
            break;

        case ir::instruction_type_t::MUL:
        case ir::instruction_type_t::DIV:
            emit_mull_or_div(self, ir_instruct);
            break;

        case ir::instruction_type_t::SQRT:
        case ir::instruction_type_t::INP:
        case ir::instruction_type_t::OUT:
        case ir::instruction_type_t::HALT:
            emit_lib_func(self, ir_instruct);
            break;

        case ir::instruction_type_t::JMP:
        case ir::instruction_type_t::CALL:
            emit_jmp_or_call(self, ir_instruct);
            break;

        case ir::instruction_type_t::RET:
            emit_ret(self);
            break;

        case ir::instruction_type_t::JE:
        case ir::instruction_type_t::JNE:
        case ir::instruction_type_t::JA:
        case ir::instruction_type_t::JAE:
        case ir::instruction_type_t::JB:
        case ir::instruction_type_t::JBE:
            emit_cond_jmp(self, ir_instruct);
            break;

        case ir::instruction_type_t::INC:
        case ir::instruction_type_t::DEC:
        case ir::instruction_type_t::SIN:
        case ir::instruction_type_t::COS:
        default:
            assert(0 && "Unsupported instruction");
            break;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::resize_if_needed(x64::code_t *self) {
    if (self->exec_buf_size + EXEC_BUF_THRESHOLD >= self->exec_buf_capacity) {
        self->exec_buf = (uint8_t*) mremap(self->exec_buf, self->exec_buf_capacity,
                                                                2*self->exec_buf_capacity, MREMAP_MAYMOVE);

        self->exec_buf_capacity *= 2;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::start_new_pass(code_t *self) {
    if (self->pass_index + 1 < TOTAL_PASS_COUNT) {
        self->exec_buf_size = 0;
    }

    self->pass_index++;
}
