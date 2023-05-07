#include <assert.h>
#include <sys/mman.h>
#include "common.h"
#include "x64_consts.h"
#include "x64.h"

//----------------------------------------------------------------------------------------------------------------------

const int PAGE_SIZE            = 4096;
const int EXEC_BUF_THRESHOLD   = 10;

const int BUF_ADDR_REGISTER    = 0b1000; // r8

const int EXTENDED_REG_MASK    = 0b1000;
const int LOWER_REG_BITS_MASK  = 0b0111;

const int IMM_MODRM_MODE_BIT        = 0b01000000;
const int DOUBLE_REG_MODRM_MODE_BIT = 0b00110100;
const int SINGLE_REG_MODRM_MODE_BIT = 0b00110000;


//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

namespace x64 {
    static void emit_push(code_t *self, ir::instruction_t *ir_instruct);

    static void emit_instruction(code_t *self, instruction_t *x64_instruct);

    static void generate_memory_arguments(instruction_t *x64_instruct, ir::instruction_t *ir_instruct);
    static inline void emit_debug_nop(code_t *self);
    static void resize_if_needed(code_t *self);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

x64::code_t *x64::code_new() {
    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    if (!self) { return nullptr;}

    self->exec_buf = (uint8_t *) mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_WRITE,
                                                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

#ifndef NDEBUG
    self->exec_buf[0] = 0xCC;
    self->exec_buf_size = 1;
#endif

    self->exec_buf_capacity = PAGE_SIZE;

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void x64::code_delete(code_t *self) {
    munmap(self->exec_buf, self->exec_buf_capacity);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

x64::code_t *x64::translate_from_ir(ir::code_t *ir_code) {
    x64::code_t *self = code_new();

    for (size_t instr_num = 0; instr_num < ir_code->size; ++instr_num) {
        switch (ir_code->instructions[instr_num].type) {
            case ir::instruction_type_t::PUSH:
                emit_push(self, ir_code->instructions + instr_num);
                break;
        }
    }

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

[[noreturn]]
void x64::execute(code_t *self) {
    asm ("jmp %0": :"r" (self->exec_buf));
}

//----------------------------------------------------------------------------------------------------------------------
// Generators
//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_push(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(self->exec_buf_size + EXEC_BUF_THRESHOLD <= self->exec_buf_capacity);
    assert(ir_instruct->type == ir::instruction_type_t::PUSH);
    emit_debug_nop(self);

    log (INFO, "emitting push");

    instruction_t x64_instruct = {};

    if (ir_instruct->need_mem_arg)
    {
        log (INFO, "\t push memory mode");
        x64_instruct.opcode = PUSH_m32;


    }
    else if (ir_instruct->need_reg_arg)
    {
        log (INFO, "\t reg arg: %s", REG_NAMES[ir_instruct->reg_num]);
        assert (ir_instruct->reg_num < 8 && "unsupported reg");

        x64_instruct.opcode = x64_OPCODES::PUSH_r32;
        x64_instruct.opcode |= ir_instruct->reg_num;
    }
    else if (ir_instruct->need_imm_arg)
    {
        log (INFO, "\t imm arg: %d", ir_instruct->need_imm_arg);

        x64_instruct.opcode     = PUSH_i32;
        x64_instruct.require_imm32  = true;
        x64_instruct.imm32      = ir_instruct->imm_arg;
    }

    emit_instruction(self, &x64_instruct);

    resize_if_needed(self);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::generate_memory_arguments(instruction_t *x64_instruct, ir::instruction_t *ir_instruct) {
    if (BUF_ADDR_REGISTER & EXTENDED_REG_MASK) {
        x64_instruct->REX = 0b01000001;
        x64_instruct->require_REX = true;
    }

    x64_instruct->require_ModRM = true;

    if (ir_instruct->need_imm_arg) {
        x64_instruct->ModRM |= IMM_MODRM_MODE_BIT;
    }

    if (ir_instruct->need_reg_arg) {
        x64_instruct->ModRM |= DOUBLE_REG_MODRM_MODE_BIT;

        x64_instruct->require_SIB = true;
        x64_instruct->SIB |= (BUF_ADDR_REGISTER & LOWER_REG_BITS_MASK) ;
        assert (ir_instruct->reg_num < 8 && "unsupported reg");
        x64_instruct->SIB |= ir_instruct->reg_num << 3;
    } else {
        x64_instruct->ModRM |= SINGLE_REG_MODRM_MODE_BIT;
    }

    log (INFO, "SIB byte: %x", *(uint8_t *) &x64_instruct->SIB);

    x64_instruct->require_imm32 = true;
    x64_instruct->imm32 = (ir_instruct->need_imm_arg) ? ir_instruct->imm_arg : 0;
    log (INFO, "imm arg value: %d", x64_instruct->imm32);
}

//----------------------------------------------------------------------------------------------------------------------

static inline void x64::emit_debug_nop(code_t *self) {
#ifndef NDEBUG
    self->exec_buf[self->exec_buf_size] = 0x90;
    self->exec_buf_size++;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_instruction(code_t *self, instruction_t *x64_instruct) {
    log (INFO, "Emitting instruction...");

    if (x64_instruct->require_REX) {
        log (INFO, "\t REX byte");
        self->exec_buf[self->exec_buf_size++] = x64_instruct->REX;
    }

    self->exec_buf[self->exec_buf_size++] = x64_instruct->opcode;

    if (x64_instruct->require_ModRM) {
        log (INFO, "\t ModRM byte");
        self->exec_buf[self->exec_buf_size++] = x64_instruct->ModRM;
    }

    if (x64_instruct->require_SIB) {
        log (INFO, "\t SIB byte");
        self->exec_buf[self->exec_buf_size++] = x64_instruct->SIB;
    }

    if (x64_instruct->require_imm32) {
        log (INFO, "\t i32 arg: %d", x64_instruct->imm32)
        *((uint32_t *) (self->exec_buf + self->exec_buf_size)) = x64_instruct->imm32;
        self->exec_buf_size += sizeof(uint32_t);
    }

    if (x64_instruct->require_imm64) {
        log (INFO, "\t i64 arg: %d", x64_instruct->imm64)
        *((uint64_t *) (self->exec_buf + self->exec_buf_size)) = x64_instruct->require_imm64;
        self->exec_buf_size += sizeof(uint64_t);
    }
}


//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static void x64::resize_if_needed(x64::code_t *self) {
    if (self->exec_buf_size + EXEC_BUF_THRESHOLD >= self->exec_buf_capacity) {
        self->exec_buf = (uint8_t*) mremap(self->exec_buf, self->exec_buf_capacity,
                                                                2*self->exec_buf_capacity, MREMAP_MAYMOVE);

        self->exec_buf_capacity *= 2;
    }
}
