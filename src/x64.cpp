#include <assert.h>
#include <sys/mman.h>
#include <math.h>
#include <stdio.h>
#include "common.h"
#include "x64_consts.h"
#include "x64.h"

// TODO Вынести stdlib в отдельный файл
// TODO еще повыносить в функции и в файлы (как минимум все emit'ы в отдельный файл)

//----------------------------------------------------------------------------------------------------------------------

#define DEBUG_BREAK

const int PAGE_SIZE            = 4096;
const int EXEC_BUF_THRESHOLD   = 15;  // 15 bytes per x64 instruction

const int BUF_ADDR_REGISTER    = x64::REG_R8;  // r8

const int TOTAL_PASSES                = 2;
const int PASS_INDEX_TO_WRITE         = 1;
const int PASS_INDEX_TO_CALC_OFFSETS  = 0;


//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

namespace x64 {
    static void emit_push_or_pop(code_t *self, ir::instruction_t *ir_instruct);
    static void emit_add_or_sub (code_t *self, ir::instruction_t *ir_instruct);
    static void emit_mull_or_div(code_t *self, ir::instruction_t *ir_instruct);
    static void emit_lib_func   (code_t *self, ir::instruction_t *ir_instruct);
    static void emit_ret(code_t *self, ir::instruction_t *ir_instruct);

    static void emit_jmp_or_call(code_t *self, ir::instruction_t *ir_instruct);
    static void emit_cond_jmp   (code_t *self, ir::instruction_t *ir_instruct);

    static void emit_instruction            (code_t *self, instruction_t *x64_instruct);
    static void emit_instruction_calc_offset(code_t *self, instruction_t *x64_instruct);
    static void emit_instruction_write      (code_t *self, instruction_t *x64_instruct);

    static void generate_memory_arguments(instruction_t *x64_instruct, ir::instruction_t *ir_instruct);
    static inline void emit_debug_nop(code_t *self);
    static void resize_if_needed(code_t *self);

    static void mul_fix_precision_multiplier(code_t *self);
    static void div_fix_precision_multiplier(code_t *self);

    static void     stdlib_out (uint64_t arg);
    static uint64_t stdlib_inp ();
    static uint64_t stdlib_sqrt(uint64_t arg);

    [[noreturn]] static void stdlib_halt();

    static uint8_t translate_cond_jump_opcode(ir::instruction_t *ir_instruct);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

x64::code_t *x64::code_new() {
    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    if (!self) { return nullptr;}

    self->exec_buf = (uint8_t *) mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_WRITE,
                                                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    self->ram_buf = (uint8_t *) mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    self->exec_buf_capacity = PAGE_SIZE;
    self->ram_buf_capacity  = PAGE_SIZE;

#ifdef DEBUG_BREAK
    self->exec_buf[0] = DEBUG_SYSCALL_BYTE; // INT 03 DEBUG BYTE
    self->exec_buf_size += 1;
#endif

    self->addr_transl = addr_transl_new();
    self->pass_index = 0;

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void x64::code_delete(code_t *self) {
    munmap(self->exec_buf, self->exec_buf_capacity);
    munmap(self->ram_buf, self->ram_buf_capacity);
    addr_transl_delete(self->addr_transl);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

x64::code_t *x64::translate_from_ir(ir::code_t *ir_code) {
    x64::code_t *self = code_new();
    ir::instruction_t *ir_instruct = ir_code->instructions;

    while (ir_instruct) {
        result_t res = addr_transl_remember_old_addr(self->addr_transl, ir_instruct->index);
        if (res == result_t::ERROR) { log(ERROR, "Failed to store old addr in self->addr_transl"); return nullptr; }

        //TODO Вынести switch в функцию
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
                emit_ret(self, ir_instruct);
                break;

            case ir::instruction_type_t::JE:
            case ir::instruction_type_t::JNE:
            case ir::instruction_type_t::JA:
            case ir::instruction_type_t::JAE:
            case ir::instruction_type_t::JB:
            case ir::instruction_type_t::JBE:
                emit_cond_jmp(self, ir_instruct);
                break;

            default:
                assert(0 && "Unsupported instruction");
                break;
        }

        ir_instruct = ir::next_insruction(ir_instruct);
    }

    return self;
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
        x64_instruct.opcode = (is_push) ? PUSH_mem : POP_mem;
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

        x64_instruct.opcode = (is_push) ? PUSH_reg : POP_reg;
        x64_instruct.opcode |= ir_instruct->reg_num;
    }
    else if (ir_instruct->need_imm_arg)
    {
        log (INFO, "\t imm arg: %d", ir_instruct->need_imm_arg);

        assert(is_push && "can't pop to imm");
        x64_instruct.opcode     = PUSH_imm;
        x64_instruct.require_imm32  = true;
        x64_instruct.imm32      = ir_instruct->imm_arg * FIXED_PRECISION_MULTIPLIER;
    }

    emit_instruction(self, &x64_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_add_or_sub(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(ir_instruct->type == ir::instruction_type_t::ADD || ir_instruct->type == ir::instruction_type_t::SUB);
    emit_debug_nop(self);

    bool is_add = ir_instruct->type == ir::instruction_type_t::ADD;
    log (INFO, "emitting add/sub, is_add: %d", is_add);

    instruction_t pop_instruct = {
            .opcode = POP_reg | REG_RAX,
    };
    emit_instruction(self, &pop_instruct);

    instruction_t additive_instruct = {
            .require_REX   = true,
            .require_ModRM = true,
            .require_SIB   = true,

            .REX    = REX_BYTE_IF_64_BIT,
            .opcode = (is_add) ? ADD_mem_reg : SUB_mem_reg,
            .ModRM  = DOUBLE_REG_MODRM_MODE_BIT,
            .SIB    = REG_RSP << SIB_BASE_OFFSET | REG_RSP << SIB_INDEX_OFFSET
    };
    emit_instruction(self, &additive_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_mull_or_div(x64::code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(ir_instruct->type == ir::instruction_type_t::MUL || ir_instruct->type == ir::instruction_type_t::DIV);
    emit_debug_nop(self);

    bool is_mul = ir_instruct->type == ir::instruction_type_t::MUL;
    log (INFO, "emitting mul/div, is_mul: %d", is_mul);

    instruction_t pop_op2_instruct = {.opcode = POP_reg | REG_RBX};
    emit_instruction(self, &pop_op2_instruct);

    instruction_t pop_op1_instruct = {.opcode = POP_reg | REG_RAX};
    emit_instruction(self, &pop_op1_instruct);

    if (!is_mul) {
        div_fix_precision_multiplier(self);
    }

    uint8_t modrm_reg_bits = (is_mul) ? MODRM_MUL_REG_BITS : MODRM_DIV_REG_BITS;
    instruction_t mult_instruct = {
            .require_ModRM = true,
            .opcode = DIVMUL_reg,
            .ModRM  = ONLY_REG_MODRM_MODE_BIT | modrm_reg_bits | REG_RBX
    };
    emit_instruction(self, &mult_instruct);

    if (is_mul) {
        mul_fix_precision_multiplier(self);
    }

    instruction_t push_res_instruct = {
            .opcode = PUSH_reg | REG_RAX,
    };
    emit_instruction(self, &push_res_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_lib_func(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    emit_debug_nop(self);

    log(INFO, "Emitting lib function...");

    instruction_t pop_arg_instruct = {
            .opcode = POP_reg | REG_RDI,
    };

    uint64_t lib_func_addr = 0;
    switch (ir_instruct->type) {
        case ir::instruction_type_t::INP:  lib_func_addr = (uint64_t) stdlib_inp;  log(INFO, "\tfunc: INP"); break;
        case ir::instruction_type_t::OUT:  lib_func_addr = (uint64_t) stdlib_out;  log(INFO, "\tfunc: OUT"); break;
        case ir::instruction_type_t::SQRT: lib_func_addr = (uint64_t) stdlib_sqrt; log(INFO, "\tfunc: SQR"); break;
        case ir::instruction_type_t::HALT: lib_func_addr = (uint64_t) stdlib_halt; log(INFO, "\tfunc: HLT"); break;
    }

    instruction_t mov_addr_instr = {
            .require_REX   = true,
            .require_ModRM = true,
            .require_imm64 = true,
            .REX           = REX_BYTE_IF_64_BIT,
            .opcode        = MOV_reg_imm,
            .ModRM         = IMM_MODRM_MODE_BIT | REG_RDI << MODRM_RM_OFFSET | REG_RAX,
            .imm64         = lib_func_addr
    };

    instruction_t call_reg = {
            .require_ModRM = true,
            .opcode = CALL_reg,
            .ModRM = ONLY_REG_MODRM_MODE_BIT | MODRM_ONLY_RM | REG_RAX
    };

    if (ir_instruct->type != ir::instruction_type_t::INP && ir_instruct->type != ir::instruction_type_t::HALT) {
        emit_instruction(self, &pop_arg_instruct);
    }

    emit_instruction(self, &mov_addr_instr);
    emit_instruction(self, &call_reg);

    if (ir_instruct->type == ir::instruction_type_t::INP || ir_instruct->type == ir::instruction_type_t::SQRT) {
        instruction_t push_res_instr = {
                .opcode = PUSH_reg | REG_RAX
        };

        emit_instruction(self, &push_res_instr);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_ret(code_t *self, ir::instruction_t *ir_instruct) {
    assert (self && ir_instruct);
    emit_debug_nop(self);

    instruction_t pop_val_instruct = {.opcode = POP_reg | REG_RBX};
    emit_instruction(self, &pop_val_instruct);

    instruction_t pop_retaddr_instruct = {.opcode = POP_reg | REG_RAX};
    emit_instruction(self, &pop_retaddr_instruct);

    instruction_t push_val_instruct = {.opcode = PUSH_reg | REG_RBX};
    emit_instruction(self, &push_val_instruct);

    instruction_t push_retaddr_instruct = {.opcode = PUSH_reg | REG_RAX};
    emit_instruction(self, &push_retaddr_instruct);

    instruction_t ret_instruct = {.opcode = RET};
    emit_instruction(self, &ret_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_jmp_or_call(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    emit_debug_nop(self);

    instruction_t mov_addr_instr = {
            .require_REX   = true,
            .require_ModRM = true,
            .require_imm64 = true,
            .REX           = REX_BYTE_IF_64_BIT,
            .opcode        = MOV_reg_imm,
            .ModRM         = IMM_MODRM_MODE_BIT | REG_RDI << MODRM_RM_OFFSET | REG_RAX,
            .imm64         = addr_transl_translate(self->addr_transl, ir_instruct->imm_arg)
    };

    emit_instruction(self, &mov_addr_instr);

    uint8_t MODRM_REG_BITS = (ir_instruct->type == ir::instruction_type_t::CALL) ? CALL_MOD_REG_BITS : JMP_MOD_REG_BITS;
    instruction_t call_reg = {
            .require_ModRM = true,
            .opcode = CALL_reg,
            .ModRM = ONLY_REG_MODRM_MODE_BIT | MODRM_REG_BITS | REG_RAX
    };

    emit_instruction(self, &call_reg);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_cond_jmp(code_t *self, ir::instruction_t *ir_instruct) {
    assert (self && ir_instruct);
    emit_debug_nop(self);


    instruction_t pop_op2_instruct = {.opcode = POP_reg | REG_RBX};
    emit_instruction(self, &pop_op2_instruct);

    instruction_t pop_op1_instruct = {.opcode = POP_reg | REG_RAX};
    emit_instruction(self, &pop_op1_instruct);

    instruction_t cmp_instruct = {
        .require_REX    = true,
        .require_ModRM  = true,
        .REX            = REX_BYTE_IF_64_BIT,
        .opcode         = CMP_reg_reg,
        .ModRM          = ONLY_REG_MODRM_MODE_BIT | REG_RBX << MODRM_RM_OFFSET | REG_RAX
    };
    emit_instruction(self, &cmp_instruct);

    const uint32_t cur_addr = (uint32_t) ((uint64_t) self->exec_buf + self->exec_buf_size) + 6;
    const uint32_t rel_pos  = (uint32_t) (addr_transl_translate(self->addr_transl, ir_instruct->imm_arg)-cur_addr);

    instruction_t cond_jmp_instruct = {
        .require_prefix = true,
        .require_imm32  = true,
        .prefix         = CONDJMP_imm_prefix,
        .opcode         = translate_cond_jump_opcode(ir_instruct),
        .imm32          = rel_pos
    };
    emit_instruction(self, &cond_jmp_instruct);
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
        x64_instruct->imm32 = ir_instruct->imm_arg * sizeof (uint64_t);
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

    addr_transl_insert_with_new_addr(self->addr_transl, (uint64_t) self->exec_buf + self->exec_buf_size);

    self->exec_buf_size += command_size;
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::emit_instruction_write(code_t *self, instruction_t *x64_instruct) {
    log (INFO, "Emitting instruction...");

    if (x64_instruct->require_REX) {
        log (INFO, "\t REX byte");
        self->exec_buf[self->exec_buf_size++] = x64_instruct->REX;
    }

    if (x64_instruct->require_prefix) {
        log (INFO, "\t REX byte");
        self->exec_buf[self->exec_buf_size++] = x64_instruct->prefix;
    }

    log(INFO, "\tOpcode: %x", x64_instruct->opcode);
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
        *((uint64_t *) (self->exec_buf + self->exec_buf_size)) = x64_instruct->imm64;
        self->exec_buf_size += sizeof(uint64_t);
    }

    resize_if_needed(self);
}


//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static void x64::mul_fix_precision_multiplier(code_t *self) {
    instruction_t load_precision_multiplier = {
            .require_REX   = true,
            .require_ModRM = true,
            .require_imm64 = true,
            .REX           = REX_BYTE_IF_64_BIT,
            .opcode        = MOV_reg_imm,
            .ModRM         = IMM_MODRM_MODE_BIT | REG_RDI << MODRM_RM_OFFSET | REG_RBX,
            .imm64         = FIXED_PRECISION_MULTIPLIER
    };
    emit_instruction(self, &load_precision_multiplier);

    instruction_t mult_instruct = {
            .require_ModRM = true,
            .opcode = DIVMUL_reg,
            .ModRM  = ONLY_REG_MODRM_MODE_BIT | MODRM_DIV_REG_BITS | REG_RBX
    };
    emit_instruction(self, &mult_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::div_fix_precision_multiplier(code_t *self) {
    instruction_t load_precision_multiplier = {
            .require_REX   = true,
            .require_ModRM = true,
            .require_imm64 = true,
            .REX           = REX_BYTE_IF_64_BIT,
            .opcode        = MOV_reg_imm,
            .ModRM         = IMM_MODRM_MODE_BIT | REG_RDI << MODRM_RM_OFFSET | REG_RBX,
            .imm64         = FIXED_PRECISION_MULTIPLIER
    };
    emit_instruction(self, &load_precision_multiplier);

    instruction_t mult_instruct = {
            .require_ModRM = true,
            .opcode = DIVMUL_reg,
            .ModRM  = ONLY_REG_MODRM_MODE_BIT | MODRM_MUL_REG_BITS | REG_RBX
    };
    emit_instruction(self, &mult_instruct);
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

static uint64_t x64::stdlib_inp() {
    printf("INPUT: ");

    const int SAFE_ALIGNMENT = 32; // scanf requires alignment to 16 bytes, but out program can't provide it
    alignas(SAFE_ALIGNMENT) uint64_t res = 0;

    scanf("%ld", &res);
    printf("\n");

    return res * FIXED_PRECISION_MULTIPLIER;
}

//----------------------------------------------------------------------------------------------------------------------

static void x64::stdlib_out(uint64_t arg) {
    printf("OUTPUT: %ld.%02ld\n", arg/FIXED_PRECISION_MULTIPLIER, arg%FIXED_PRECISION_MULTIPLIER);
}

//----------------------------------------------------------------------------------------------------------------------

static uint64_t x64::stdlib_sqrt(uint64_t arg) {
    double d_arg = (double) arg *  FIXED_PRECISION_MULTIPLIER;
    return (uint64_t) sqrt(d_arg);
}

//----------------------------------------------------------------------------------------------------------------------

[[noreturn]]
static void x64::stdlib_halt() {
    abort();
}

//----------------------------------------------------------------------------------------------------------------------

#define TRANSLATE_TYPE_TO_OPCODE(type, opcode_const)                                \
    case ir::instruction_type_t::type:                                              \
        log (INFO, "\tCond type: " #type " resulted in " #opcode_const " opcode");  \
        opcode = x64::opcode_const; break;

static uint8_t x64::translate_cond_jump_opcode(ir::instruction_t *ir_instruct) {
    uint8_t opcode = 0;
    log(INFO, "Decoding conditional IF...");

    switch (ir_instruct->type) {
        TRANSLATE_TYPE_TO_OPCODE(JE,    JE_imm)
        TRANSLATE_TYPE_TO_OPCODE(JNE,   JNE_imm)
        TRANSLATE_TYPE_TO_OPCODE(JA,    JA_imm)
        TRANSLATE_TYPE_TO_OPCODE(JAE,   JNB_imm)
        TRANSLATE_TYPE_TO_OPCODE(JB,    JNAE_imm)
        TRANSLATE_TYPE_TO_OPCODE(JBE,   JNA_imm)
    }

    return opcode;
}

#undef TRANSLATE_TYPE_TO_OPCODE