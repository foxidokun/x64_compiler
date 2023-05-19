#include "ast_converter_generators.h"
#include "ast_converter_common.h"

// -------------------------------------------------------------------------------------------------
// Consts
// -------------------------------------------------------------------------------------------------

enum REGISTERS {
    REG_RAX = 0,
    REG_RBX = 1,
    REG_RCX = 2,
    REG_RDX = 3,
};

const int BASE_MEM_REG = REG_RDX;

// -------------------------------------------------------------------------------------------------
// Prototypes
// -------------------------------------------------------------------------------------------------

namespace ir {
    static result_t convert_op              (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_if              (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_while           (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_func_call       (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_func_def        (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_ret             (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_and             (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_or              (converter_t *converter, tree::node_t *node, code_t *ir_code);
    static result_t convert_logical_operands(converter_t *converter, tree::node_t *node, code_t *ir_code);

    static int convert_func_call_args(converter_t *converter, tree::node_t *node, code_t *ir_code);
    static int convert_func_def_args (converter_t *converter, tree::node_t *node, code_t *ir_code);

    static void emit_push_true_false(converter_t *converter, instruction_type_t jmp_type, code_t *ir_code);
    static void emit_out(converter_t *converter, code_t *ir_code);
    static result_t emit_assig(converter_t *converter, uint64_t var_num, code_t *ir_code);

    static result_t get_var_code(converter_t *converter, int var_num, code_t *ir_code);
    static void register_var(converter_t *converter, int var_num);

    static void clear_local_vars (converter_t *converter);

    static void emit_instruction_wrapper(converter_t *converter, code_t *ir_code, instruction_type_t type, bool has_mem_arg,
                                         bool has_reg_arg, bool has_imm_arg, unsigned char reg, uint64_t imm);
}

// -------------------------------------------------------------------------------------------------
// Macros
// -------------------------------------------------------------------------------------------------

// M -- Inderect addressing
// R -- Register argument
// I -- Immediate argument
#define EMIT_NONE(TYPE)            emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE, false, false, false,   0,   0)
#define EMIT_I(TYPE, IMM)          emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE, false, false, true,    0, IMM)
#define EMIT_R(TYPE, REG)          emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE, false, true,  false, REG,   0)
#define EMIT_R_I(TYPE, REG, IMM)   emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE, false, true,  true,  REG, IMM)
#define EMIT_M_I(TYPE, IMM)        emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE,  true, false, true,    0, IMM)
#define EMIT_M_R(TYPE, REG)        emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE,  true, true,  false, REG,   0)
#define EMIT_M_R_I(TYPE, REG, IMM) emit_instruction_wrapper(converter, ir_code, instruction_type_t::TYPE,  true, true,  true,  REG, IMM)

#define GET_LABEL(label, index)                                                         \
    uint label     = get_label_index(converter);                                        \
    uint64_t index = addr_transl_translate(converter->indexed_label_transl, label);     \

// -------------------------------------------------------------------------------------------------
// Protected
// -------------------------------------------------------------------------------------------------

void ir::emit_code_begin(converter_t *converter, code_t *ir_code) {
    EMIT_I(PUSH, 0);
    EMIT_R(POP, REG_RDX);
}

// -------------------------------------------------------------------------------------------------

void ir::emit_code_end(converter_t *converter, code_t *ir_code) {
    EMIT_NONE(HALT);
}

// -------------------------------------------------------------------------------------------------

result_t ir::subtree_convert(converter_t *converter, tree::node_t *node, code_t *ir_code, bool result_used) {
    assert (converter && ir_code);

    if (node == nullptr) {
        return result_t::OK;
    }

    switch (node->type)
    {
        case tree::node_type_t::FICTIOUS:
            UNWRAP_ERROR(subtree_convert(converter, node->left,  ir_code, false));
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code, false));
            break;

        case tree::node_type_t::VAL:
            EMIT_I(PUSH, node->data);
            break;

        case tree::node_type_t::VAR:
            EMIT_NONE(PUSH);
            UNWRAP_ERROR(get_var_code(converter, node->data, ir_code));
            break;

        case tree::node_type_t::VAR_DEF:
            register_var(converter, node->data);
            break;

        case tree::node_type_t::OP:
            UNWRAP_ERROR(convert_op(converter, node, ir_code));
            break;

        case tree::node_type_t::IF:
            UNWRAP_ERROR(convert_if(converter, node, ir_code));
            break;

        case tree::node_type_t::WHILE:
            UNWRAP_ERROR(convert_while(converter, node, ir_code));
            break;

        case tree::node_type_t::FUNC_DEF:
            UNWRAP_ERROR(convert_func_def(converter, node, ir_code));
            break;

        case tree::node_type_t::FUNC_CALL:
            UNWRAP_ERROR(convert_func_call(converter, node, ir_code));
            if (!result_used) {
                EMIT_R(POP, REG_RAX);
            }
            break;

        case tree::node_type_t::RETURN:
            UNWRAP_ERROR(convert_ret(converter, node, ir_code));
            break;

        case tree::node_type_t::ELSE:
            assert (0 && "Already compiled in IF node");

        case tree::node_type_t::NOT_SET:
            assert (0 && "Invalid node");

        default:
        log (ERROR, "Node type: %d\n", node->type);
            assert (0 && "Unexpected node");
    }

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

#define EMIT_BINARY_OP(opcode)                                       \
    UNWRAP_ERROR(subtree_convert(converter, node->left,  ir_code));  \
    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));  \
    EMIT_NONE(opcode);

#define EMIT_PUSH_TRUE_FALSE(jump_opcode) emit_push_true_false(converter, ir::instruction_type_t::jump_opcode, ir_code)

#define EMIT_COMPARATOR(opcode)                                      \
    UNWRAP_ERROR(subtree_convert(converter, node->left,  ir_code));  \
    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));  \
    EMIT_PUSH_TRUE_FALSE (opcode)


result_t ir::convert_op(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter != nullptr && "invalid pointer");
    assert (node      != nullptr && "invalid pointer");
    assert (ir_code   != nullptr && "invalid pointer");

    assert (node->type == tree::node_type_t::OP);

    switch ((tree::op_t) node->data)
    {
        case tree::op_t::ADD: EMIT_BINARY_OP(ADD); break;
        case tree::op_t::SUB: EMIT_BINARY_OP(SUB); break;
        case tree::op_t::MUL: EMIT_BINARY_OP(MUL); break;
        case tree::op_t::DIV: EMIT_BINARY_OP(DIV); break;

        case tree::op_t::SQRT:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            EMIT_NONE(SQRT);
            break;

        case tree::op_t::SIN:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            EMIT_NONE(SIN);
            break;

        case tree::op_t::COS:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            EMIT_NONE(COS);
            break;

        case tree::op_t::OUTPUT:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            emit_out(converter, ir_code);
            break;

        case tree::op_t::ASSIG:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            emit_assig(converter, node->left->data, ir_code);
            break;

        case tree::op_t::INPUT:
            EMIT_NONE(INP);
            break;

        case tree::op_t::EQ:  EMIT_COMPARATOR (JE);  break;
        case tree::op_t::GT:  EMIT_COMPARATOR (JA);  break;
        case tree::op_t::LT:  EMIT_COMPARATOR (JB);  break;
        case tree::op_t::GE:  EMIT_COMPARATOR (JAE); break;
        case tree::op_t::LE:  EMIT_COMPARATOR (JBE); break;
        case tree::op_t::NEQ: EMIT_COMPARATOR (JNE); break;

        case tree::op_t::NOT:
            UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));
            EMIT_I(PUSH, 0);
            EMIT_PUSH_TRUE_FALSE(JE);
            break;

        case tree::op_t::AND:
            UNWRAP_ERROR(convert_and(converter, node, ir_code));
            break;

        case tree::op_t::OR:
            UNWRAP_ERROR(convert_or(converter, node, ir_code));
            break;

        default:
            assert (0 && "Unexpected op type");
    }

    return result_t::OK;
}

#undef EMIT_BINARY_OP
#undef EMIT_COMPARATOR

// -------------------------------------------------------------------------------------------------

result_t ir::convert_if(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter != nullptr && "invalid pointer");
    assert (node      != nullptr && "invalid pointer");
    assert (ir_code   != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::IF && "Invalid call");

    UNWRAP_ERROR(subtree_convert(converter, node->left, ir_code));
    EMIT_I(PUSH, 0);

    // IF end label
    GET_LABEL(end_label, end_ir_indx);


    if (node->right->left != nullptr) {      // Check if there is else branch
        GET_LABEL(else_label, else_ir_indx); // Pointer to else block

        EMIT_I(JE, else_ir_indx);                                                    // IF cond == 0 jmp to else
        UNWRAP_ERROR(subtree_convert(converter, node->right->left, ir_code));  //  ... if branch ...
        EMIT_I(JMP, end_ir_indx);                                                    // JMP out of IF
        register_numeric_label(converter, else_label);                     // else_label:
        UNWRAP_ERROR(subtree_convert(converter, node->right->right, ir_code)); //  ... else branch ...
    } else {
        EMIT_I(JE, end_ir_indx);                                                     // IF cond == 0 jmp to end
        UNWRAP_ERROR(subtree_convert(converter, node->right->right, ir_code)); //  ... if branch ...
    }

    // end_label:
    register_numeric_label(converter, end_label);

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

result_t ir::convert_while(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter && node && ir_code && "Invalid pointers");
    assert (node->type == tree::node_type_t::WHILE && "Invalid call");

    GET_LABEL(while_beg_label, while_beg_ir_indx);
    GET_LABEL(while_end_label, while_end_ir_indx);

    register_numeric_label(converter, while_beg_label);         // while_beg:
    UNWRAP_ERROR(subtree_convert(converter, node->left, ir_code));  // ... condition ...

    EMIT_I(PUSH, 0);                                                      //
    EMIT_I(JE, while_end_ir_indx);                                        // if cond == 0 jump to while_end

    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code)); // ... while body ...

    EMIT_I(JMP, while_beg_ir_indx);                                       // jmp to while_beg
    register_numeric_label(converter, while_end_label);         // while_end:

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

result_t ir::convert_func_def(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter && node && ir_code && "Invalid pointers");
    assert (node->type == tree::node_type_t::FUNC_DEF && "Invalid call");

    converter->global_frame_size_store = converter->frame_size;
    converter->frame_size = 0;
    converter->in_func = true;

    GET_LABEL(func_def_end_label, func_def_end_ir_indx);

    EMIT_I (JMP, func_def_end_ir_indx);                                     // jmp func_def_end
    register_function_label(converter, node->data);               // func:

    EMIT_I(FUNC_BEGIN, node->data);                                         // *service node*

    convert_func_def_args(converter, node->left, ir_code);            // ... load func args ...
    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code));   // ... func body ...

    register_numeric_label(converter, func_def_end_label);        // func_def_end:

    converter->in_func = false;
    clear_local_vars (converter);
    converter->frame_size = converter->global_frame_size_store;

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

result_t ir::convert_func_call(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter && node && ir_code && "Invalid pointers");
    assert (node->type == tree::node_type_t::FUNC_CALL && "Invalid call");

    int arg_counter = convert_func_call_args(converter, node->right, ir_code);   // ... load func args into stack ...

    EMIT_R(PUSH, REG_RDX);                                                             //
    EMIT_I(PUSH, converter->frame_size);                                               //
    EMIT_NONE(ADD);                                                                    // * Increment frame size *
    EMIT_R(POP, REG_RDX);                                                              // add rbx, %frame_size

    for (int i = arg_counter - 1; i >= 0; --i) {
        EMIT_M_R_I(POP, REG_RDX, i);                                                   // ... load func args into frame ...
    }

    EMIT_I(CALL, addr_transl_translate(converter->func_label_transl, node->data)); // call func%d

    EMIT_R(PUSH, REG_RDX);                                                              //
    EMIT_I(PUSH, converter->frame_size);                                                //
    EMIT_NONE(SUB);                                                                     // * Decrement frame size *
    EMIT_R(POP, REG_RDX);                                                               // sub rbx, %frame_size

    return result_t::OK;
}

result_t ir::convert_ret(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert(converter && node && ir_code);

    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code, true)); // ... ret arg ...
    EMIT_R(POP, REG_RAX);   //
    EMIT_R(POP, REG_RBX);   // * Exchange ret addr and func res
    EMIT_R(PUSH, REG_RAX);  // pop rax, rbx
    EMIT_R(PUSH, REG_RBX);  // push rax, rbx

    EMIT_NONE(RET);         // ret

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------
// Static
// -------------------------------------------------------------------------------------------------

static int ir::convert_func_call_args(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter != nullptr && "invalid pointer");
    assert (ir_code   != nullptr && "invalid pointer");

    int args_counter = 0;

    if (node == nullptr) {
        return args_counter;
    }

    if (node->type != tree::node_type_t::FICTIOUS) {
        args_counter++;
        subtree_convert(converter, node, ir_code);
        return args_counter;
    }

    if (node->left != nullptr)
    {
        if (node->left->type == tree::node_type_t::FICTIOUS) {
            args_counter += convert_func_call_args(converter, node->left, ir_code);
        } else {
            args_counter++;
            subtree_convert(converter, node->left, ir_code);
        }
    }

    if (node->right != nullptr)
    {
        if (node->right->type == tree::node_type_t::FICTIOUS) {
            args_counter += convert_func_call_args(converter, node->right, ir_code);
        } else {
            args_counter++;
            subtree_convert(converter, node->right, ir_code);
        }
    }

    return args_counter;
}

// -------------------------------------------------------------------------------------------------

static int ir::convert_func_def_args(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    assert (converter != nullptr && "invalid pointer");
    assert (ir_code   != nullptr && "invalid pointer");

    if (node == nullptr) {
        return 0;
    }

    int num_of_args = 0;

    if (node->type == tree::node_type_t::FICTIOUS) {
        if (node->left != nullptr) {
            num_of_args += convert_func_def_args(converter, node->left, ir_code);
        }

        if (node->right != nullptr) {
            num_of_args += convert_func_def_args(converter, node->right, ir_code);
        }
    } else if (node->type == tree::node_type_t::VAR) {
        register_var (converter, node->data);
        num_of_args = 1;
    } else {
        assert (0 && "Broken func def params subtree");
    }

    return num_of_args;
}

// -------------------------------------------------------------------------------------------------

static void ir::emit_push_true_false(converter_t *converter, instruction_type_t jmp_type, code_t *ir_code) {
    GET_LABEL(push_one_label, push_one_ir_indx);
    GET_LABEL(end_label, end_ir_indx);

    emit_instruction_wrapper(converter, ir_code, jmp_type, false, false, true,
                                                                                        0, push_one_ir_indx);
                                                                    // Jxx push_one
    EMIT_I(PUSH, 0);                                                // push 0
    EMIT_I(JMP, end_ir_indx);                                       // jmp end

    register_numeric_label(converter, push_one_label);    // push_one:

    EMIT_I(PUSH, 1);                                                // push 1

    register_numeric_label(converter, end_label);         // end:
}

// -------------------------------------------------------------------------------------------------

static void ir::emit_out(converter_t *converter, code_t *ir_code) {
    assert(converter && ir_code);

    // Duplicate output
    EMIT_R(POP,  REG_RAX);
    EMIT_R(PUSH, REG_RAX);
    EMIT_R(PUSH, REG_RAX);
    EMIT_NONE(OUT);
}

// -------------------------------------------------------------------------------------------------

static result_t ir::emit_assig(converter_t *converter, uint64_t var_num, code_t *ir_code) {
    assert(converter && ir_code);

    EMIT_NONE(POP);
    UNWRAP_ERROR(get_var_code(converter, var_num, ir_code));

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

static result_t ir::convert_logical_operands(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    UNWRAP_ERROR(subtree_convert(converter, node->left, ir_code));  // ... compute left operand ...
    EMIT_I(PUSH, 0);                                                      // * boolify lhs *
    EMIT_PUSH_TRUE_FALSE(JNE);                                            // push (lhs != 0)
    UNWRAP_ERROR(subtree_convert(converter, node->right, ir_code)); // ... compute right operand ...
    EMIT_I(PUSH, 0);                                                      // * boolify rhs *
    EMIT_PUSH_TRUE_FALSE(JNE);                                            // push (rhs != 0)
}


// -------------------------------------------------------------------------------------------------

result_t ir::convert_or(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    convert_logical_operands(converter, node, ir_code);
    EMIT_NONE(ADD); // If at least one != 0, it will be != 0

    return result_t::OK;
}

// -------------------------------------------------------------------------------------------------

result_t ir::convert_and(converter_t *converter, tree::node_t *node, code_t *ir_code) {
    convert_logical_operands(converter, node, ir_code);
    EMIT_PUSH_TRUE_FALSE(JE); // If both are != 0, it will be != 0

    return result_t::OK;
}

#undef EMIT_PUSH_TRUE_FALSE

// -------------------------------------------------------------------------------------------------

static result_t ir::get_var_code(converter_t *converter, int var_num, code_t *ir_code) {
    assert (converter   != nullptr && "Invalid pointer");
    assert (ir_code != nullptr && "Invalid pointer");

    instruction_t new_instruction = {};

    for (unsigned int i = 0; i < converter->local_vars.size; ++i) {
        if (converter->local_vars.name_indexes[i] == var_num) {
            new_instruction.need_mem_arg = true;
            new_instruction.need_reg_arg = true;
            new_instruction.need_imm_arg = true;
            new_instruction.reg_num      = BASE_MEM_REG;
            new_instruction.imm_arg      = i;

            update_last_instruction_args(converter, ir_code, &new_instruction);

            return result_t::OK;
        }
    }

    for (unsigned int i = 0; i < converter->global_vars.size; ++i) {
        if (converter->global_vars.name_indexes[i] == var_num) {
            new_instruction.need_mem_arg = true;
            new_instruction.need_reg_arg = false;
            new_instruction.need_imm_arg = true;
            new_instruction.imm_arg      = i;

            update_last_instruction_args(converter, ir_code, &new_instruction);
            return result_t::OK;
        }
    }

    log (ERROR, "FAILED to get var code %d", var_num);
    return result_t::ERROR;
}

// -------------------------------------------------------------------------------------------------

static void ir::register_var(converter_t *converter, int var_num) {
    assert (converter != nullptr && "invalid pointer");

    vars_t *vars = nullptr;

    if (converter->in_func) {
        vars = &converter->local_vars;
    } else {
        vars = &converter->global_vars;
    }

    if (vars->size == vars->capacity)
    {
        vars->name_indexes = (int *) realloc (vars->name_indexes, 2 * vars->capacity * sizeof (int));
        vars->capacity    *= 2;
    }

    vars->name_indexes[vars->size] = var_num;
    vars->size++;

    converter->frame_size++;
}

// -------------------------------------------------------------------------------------------------

static void ir::clear_local_vars(converter_t *converter) {
    assert (converter != nullptr && "invalid pointer");

    converter->local_vars.size = 0;
}

// -------------------------------------------------------------------------------------------------

static void ir::emit_instruction_wrapper(converter_t *converter, code_t *ir_code, instruction_type_t type, bool has_mem_arg,
                                                bool has_reg_arg, bool has_imm_arg, unsigned char reg, uint64_t imm) {
    instruction_t instruct = {
            .type         = type,
            .need_imm_arg = has_imm_arg,
            .need_reg_arg = has_reg_arg,
            .need_mem_arg = has_mem_arg,
            .reg_num      = reg,
            .imm_arg      = imm
    };

    emit_instruction(converter, ir_code, &instruct);
}
