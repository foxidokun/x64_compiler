#include <assert.h>
#include <sys/mman.h>
#include "common.h"
#include "x64_consts.h"
#include "x64.h"

//----------------------------------------------------------------------------------------------------------------------

const int PAGE_SIZE            = 4096;
const int EXEC_BUF_THRESHOLD   = 10;

const int BUF_ADDR_REGISTER    = x64::REG_R8;  // r8



//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

namespace x64 {
    static void emit_push_or_pop(code_t *self, ir::instruction_t *ir_instruct);
    static void emit_add_or_sub(code_t *self, ir::instruction_t *ir_instruct);

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
            case ir::instruction_type_t::POP:
                emit_push_or_pop(self, ir_code->instructions + instr_num);
                break;
            case ir::instruction_type_t::ADD:
            case ir::instruction_type_t::SUB:
                emit_add_or_sub(self, ir_code->instructions + instr_num);
                break;
            default:
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

static void x64::emit_push_or_pop(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(ir_instruct->type == ir::instruction_type_t::PUSH || ir_instruct->type == ir::instruction_type_t::POP);
    emit_debug_nop(self);

    log (INFO, "emitting push/pop");

    instruction_t x64_instruct = {};
    bool is_push = ir_instruct->type == ir::instruction_type_t::PUSH;

    if (ir_instruct->need_mem_arg)
    {
        log (INFO, "\t push/pop memory mode");
        x64_instruct.opcode = (is_push) ? PUSH_m32 : POP_m32;
        generate_memory_arguments(&x64_instruct, ir_instruct);

        if (is_push) {
            x64_instruct.ModRM |= PUSH_MOD_REG_BITS;
        } else {
            x64_instruct.ModRM |= POP_MOD_REG_BITS;
        }
    }
    else if (ir_instruct->need_reg_arg)
    {
        log (INFO, "\t reg arg: %s", REG_NAMES[ir_instruct->reg_num]);
        assert (ir_instruct->reg_num < 8 && "unsupported reg");

        x64_instruct.opcode = (is_push) ? PUSH_r32 : POP_r32;
        x64_instruct.opcode |= ir_instruct->reg_num;
    }
    else if (ir_instruct->need_imm_arg)
    {
        log (INFO, "\t imm arg: %d", ir_instruct->need_imm_arg);

        assert(is_push && "can't pop to imm");
        x64_instruct.opcode     = PUSH_i32;
        x64_instruct.require_imm32  = true;
        x64_instruct.imm32      = ir_instruct->imm_arg;
    }

    emit_instruction(self, &x64_instruct);

    resize_if_needed(self);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_add_or_sub(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(ir_instruct->type == ir::instruction_type_t::ADD || ir_instruct->type == ir::instruction_type_t::SUB);

    bool is_add = ir_instruct->type == ir::instruction_type_t::ADD;

    instruction_t pop_instruct = {
            .opcode = POP_r32 | REG_RAX,
    };

    instruction_t additive_instruct = {
            .require_REX = true,
            .require_ModRM = true,
            .require_SIB = true,

            .REX    = REX_BYTE_IF_64_BIT,
            .opcode = (is_add) ? ADD_m64_r64 : SUB_m64_r64,
            .ModRM  = DOUBLE_REG_MODRM_MODE_BIT,
            .SIB    = REG_RSP << SIB_BASE_OFFSET | REG_RSP << SIB_INDEX_OFFSET
    };

    emit_instruction(self, &pop_instruct);
    emit_instruction(self, &additive_instruct);

    resize_if_needed(self);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::generate_memory_arguments(instruction_t *x64_instruct, ir::instruction_t *ir_instruct) {
    if (BUF_ADDR_REGISTER & EXTENDED_REG_MASK) {
        x64_instruct->REX = REX_BYTE_IF_EXTENDED;
        x64_instruct->require_REX = true;
    }

    x64_instruct->require_ModRM = true;

    if (ir_instruct->need_imm_arg) {
        x64_instruct->ModRM |= IMM_MODRM_MODE_BIT;
        x64_instruct->require_imm32 = true;
        x64_instruct->imm32 = ir_instruct->imm_arg;
    }

    if (ir_instruct->need_reg_arg) {
        x64_instruct->ModRM |= DOUBLE_REG_MODRM_MODE_BIT;

        x64_instruct->require_SIB = true;
        x64_instruct->SIB |= (BUF_ADDR_REGISTER & LOWER_REG_BITS_MASK) ;
        assert (ir_instruct->reg_num < 8 && "unsupported reg");
        x64_instruct->SIB |= ir_instruct->reg_num << SIB_INDEX_OFFSET;
    } else {
        x64_instruct->ModRM |= SINGLE_REG_MODRM_MODE_BIT;
    }

    log (INFO, "SIB byte: %x", *(uint8_t *) &x64_instruct->SIB);
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
