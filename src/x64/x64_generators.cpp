#include "x64_consts.h"
#include "x64_common.h"
#include "x64_stdlib.h"
#include "x64_generators.h"

//----------------------------------------------------------------------------------------------------------------------

const int BUF_ADDR_REGISTER    = x64::REG_R8;  // r8

//----------------------------------------------------------------------------------------------------------------------
// Static Prototypes
//----------------------------------------------------------------------------------------------------------------------

namespace x64 {
    static void generate_memory_arguments(instruction_t *x64_instruct, ir::instruction_t *ir_instruct);
    static inline void emit_debug_nop(code_t *self);

    static void mul_fix_precision_multiplier(code_t *self);
    static void div_fix_precision_multiplier(code_t *self);

    static uint8_t translate_cond_jump_opcode(ir::instruction_t *ir_instruct);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

void x64::emit_push_or_pop(code_t *self, ir::instruction_t *ir_instruct) {
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

void x64::emit_add_or_sub(code_t *self, ir::instruction_t *ir_instruct) {
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

void x64::emit_mull_or_div(x64::code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    assert(ir_instruct->type == ir::instruction_type_t::MUL || ir_instruct->type == ir::instruction_type_t::DIV);
    emit_debug_nop(self);

    bool is_mul = ir_instruct->type == ir::instruction_type_t::MUL;
    log (INFO, "emitting mul/div, is_mul: %d", is_mul);

    instruction_t pop_op2_instruct = {.opcode = POP_reg | REG_RBX};
    emit_instruction(self, &pop_op2_instruct);

    instruction_t pop_op1_instruct = {.opcode = POP_reg | REG_RAX};
    emit_instruction(self, &pop_op1_instruct);

    instruction_t extend_rax_to_rdxrax_instruct = {
            .require_REX = true,
            .REX         = REX_BYTE_IF_64_BIT,
            .opcode      = 0x99
    };
    emit_instruction(self, &extend_rax_to_rdxrax_instruct);

    if (!is_mul) {
        div_fix_precision_multiplier(self);
    }

    uint8_t modrm_reg_bits = (is_mul) ? MODRM_MUL_REG_BITS : MODRM_DIV_REG_BITS;
    instruction_t mult_instruct = {
            .require_REX   = true,
            .require_ModRM = true,
            .REX    = REX_BYTE_IF_64_BIT,
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

void x64::emit_lib_func(code_t *self, ir::instruction_t *ir_instruct) {
    assert(self && ir_instruct);
    emit_debug_nop(self);

    log(INFO, "Emitting lib function...");

    if (ir_instruct->type != ir::instruction_type_t::INP && ir_instruct->type != ir::instruction_type_t::HALT) {
        instruction_t pop_arg_instruct = {
                .opcode = POP_reg | REG_RDI,
        };

        emit_instruction(self, &pop_arg_instruct);
    }

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
    emit_instruction(self, &mov_addr_instr);

    instruction_t save_r8    = {.require_REX = true, .REX = REX_BYTE_IF_EXTENDED, .opcode = PUSH_reg};

    emit_instruction(self, &save_r8);
    instruction_t call_reg = {
            .require_ModRM = true,
            .opcode = CALL_reg,
            .ModRM = ONLY_REG_MODRM_MODE_BIT | MODRM_ONLY_RM | REG_RAX
    };
    emit_instruction(self, &call_reg);

    instruction_t restore_r8 = {.require_REX = true, .REX = REX_BYTE_IF_EXTENDED, .opcode = POP_reg};
    emit_instruction(self, &restore_r8);


    if (ir_instruct->type == ir::instruction_type_t::INP || ir_instruct->type == ir::instruction_type_t::SQRT) {
        instruction_t push_res_instr = {
                .opcode = PUSH_reg | REG_RAX
        };

        emit_instruction(self, &push_res_instr);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void x64::emit_ret(code_t *self, ir::instruction_t *ir_instruct) {
    assert (self && ir_instruct);
    emit_debug_nop(self);

    instruction_t ret_instruct = {.opcode = RET};
    emit_instruction(self, &ret_instruct);
}

//----------------------------------------------------------------------------------------------------------------------

void x64::emit_jmp_or_call(code_t *self, ir::instruction_t *ir_instruct) {
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

void x64::emit_cond_jmp(code_t *self, ir::instruction_t *ir_instruct) {
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
// Static Functions
//----------------------------------------------------------------------------------------------------------------------

static inline void x64::emit_debug_nop(code_t *self) {
#ifndef NDEBUG
    self->exec_buf[self->exec_buf_size] = 0x90;
    self->exec_buf_size++;
#endif
}

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
            .require_REX   = true,
            .require_ModRM = true,
            .REX    = REX_BYTE_IF_64_BIT,
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
            .ModRM         = IMM_MODRM_MODE_BIT | REG_RDI << MODRM_RM_OFFSET | REG_RCX,
            .imm64         = FIXED_PRECISION_MULTIPLIER
    };
    emit_instruction(self, &load_precision_multiplier);

    instruction_t mult_instruct = {
            .require_REX   = true,
            .require_ModRM = true,
            .REX    = REX_BYTE_IF_64_BIT,
            .opcode = DIVMUL_reg,
            .ModRM  = ONLY_REG_MODRM_MODE_BIT | MODRM_MUL_REG_BITS | REG_RCX
    };
    emit_instruction(self, &mult_instruct);
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