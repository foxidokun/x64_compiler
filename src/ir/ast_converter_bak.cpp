#include <assert.h>
#include <cstdlib>
#include "../lib/address_translator.h"
#include "../common.h"
#include "ast_converter.h"

struct vars_t {
    int *name_indexes;

    unsigned int size;
    unsigned int capacity;
};

struct compiler_t
{
    vars_t global_vars;
    vars_t local_vars;

    bool in_func;
    int global_frame_size_store;
    int frame_size;

    int cur_label_index;

    addr_transl_t *addr_transl;
    uint pass_index;
};

// -------------------------------------------------------------------------------------------------

const int TOTAL_PASS_COUNT            = 2;
const int PASS_INDEX_TO_WRITE         = 1;
const int PASS_INDEX_TO_CALC_OFFSETS  = 0;

// -------------------------------------------------------------------------------------------------

void ctor (compiler_t *self);
void dtor (compiler_t *self);

// -------------------------------------------------------------------------------------------------

const int DEFAULT_VARS_CAPACITY = 16;
const int BUF_SIZE              = 64;
const int VAR_BUF_SIZE          = 16;

// -------------------------------------------------------------------------------------------------

static bool subtree_compile        (compiler_t *compiler, tree::node_t *node, bool result_used = true);
static bool compile_op             (compiler_t *compiler, tree::node_t *node);
static bool compile_if             (compiler_t *compiler, tree::node_t *node);
static bool compile_while          (compiler_t *compiler, tree::node_t *node);
static bool compile_func_def       (compiler_t *compiler, tree::node_t *node);
static int  compile_func_def_args  (compiler_t *compiler, tree::node_t *node);
static bool compile_func_call      (compiler_t *compiler, tree::node_t *node);
static int  compile_func_call_args (compiler_t *compiler, tree::node_t *node);

static int  ctor_vars (vars_t *vars);
static void dtor_vars (vars_t *vars);

static void register_var     (compiler_t *compiler, int number);
static bool get_var_code     (compiler_t *compiler, int number, char *code);
static void clear_local_vars (compiler_t *compiler);

static int  get_label_index  (compiler_t *compiler);

// -------------------------------------------------------------------------------------------------

#define TRY(cond)       \
{                       \
    if (!(cond))        \
    {                   \
        return false;   \
    }                   \
}

// -------------------------------------------------------------------------------------------------

//void ir::ctor (compiler_t *self)
//{
//    assert (self != nullptr && "invalid pointer");
//
//    ctor_vars (&self->global_vars);
//    ctor_vars (&self->local_vars);
//
//    self->cur_label_index         = 0;
//    self->frame_size              = 0;
//    self->global_frame_size_store = 0;
//
//    self->addr_transl = addr_transl_new();
//    self->pass_index = 0;
//}
//
//void ir::dtor (compiler_t *self)
//{
//    assert (self != nullptr && "invalid pointer");
//
//    dtor_vars (&self->global_vars);
//    dtor_vars (&self->local_vars);
//}

// -------------------------------------------------------------------------------------------------

//ir::code_t *ir::compile (tree::node_t *node)
//{
//    assert (node   != nullptr && "invalid pointer");
//
//    compiler_t compiler = {};
//    ctor (&compiler);
//
//    char buf[BUF_SIZE] = "";
//
////    EMIT ("push 0");
////    EMIT ("pop rdx ; Init rdx");
////    EMIT ("  ");
////    EMIT ("  ");
////    EMIT ("; Here we go again");
//
//    TRY (subtree_compile (&compiler, node));
//
////    EMIT ("halt");
//
//    dtor (&compiler);
//    return true;
//}

// -------------------------------------------------------------------------------------------------

//static bool subtree_compile (compiler_t *compiler, tree::node_t *node, FILE *stream, bool result_used)
//{
//    assert (stream != nullptr && "invalid pointer");
//
//    if (node == nullptr) {
//        return true;
//    }
//
//    char buf[BUF_SIZE] = "";
//    char var_code_buf[VAR_BUF_SIZE] = "";
//
//    switch (node->type)
//    {
//        case tree::node_type_t::FICTIOUS:
//            TRY (subtree_compile (compiler, node->left,  stream, false));
//            TRY (subtree_compile (compiler, node->right, stream, false));
//            break;
//
//        case tree::node_type_t::VAL:
//            EMIT ("push %d ; val node", node->data);
//            break;
//
//        case tree::node_type_t::VAR:
//            TRY (get_var_code (compiler, node->data, var_code_buf));
//            EMIT ("push %s", var_code_buf);
//            break;
//
//        case tree::node_type_t::VAR_DEF:
//            register_var (compiler, node->data);
//            break;
//
//        case tree::node_type_t::OP:
//            TRY (compile_op (compiler, node, stream));
//            break;
//
//        case tree::node_type_t::IF:
//            TRY (compile_if (compiler, node, stream));
//            break;
//
//        case tree::node_type_t::WHILE:
//            TRY (compile_while (compiler, node, stream));
//            break;
//
//        case tree::node_type_t::FUNC_DEF:
//            TRY (compile_func_def (compiler, node, stream));
//            break;
//
//        case tree::node_type_t::FUNC_CALL:
//            TRY (compile_func_call (compiler, node, stream));
//            if (!result_used) {
//                EMIT ("pop rax ;Remove unused val")
//            }
//            break;
//
//        case tree::node_type_t::RETURN:
//            TRY (subtree_compile (compiler, node->right,  stream, true));
//            EMIT ("ret");
//            break;
//
//        case tree::node_type_t::ELSE:
//            assert (0 && "Already compiled in IF node");
//
//        case tree::node_type_t::NOT_SET:
//            assert (0 && "Invalid node");
//
//        default:
//            log (ERROR, "Node type: %d\n", node->type);
//            assert (0 && "Unexpected node");
//    }
//
//    return true;
//}

// -------------------------------------------------------------------------------------------------

//#define EMIT_BINARY_OP(opcode)                              \
//    TRY (subtree_compile (compiler, node->left,  stream));  \
//    TRY (subtree_compile (compiler, node->right, stream));  \
//    EMIT (opcode);
//
//#define EMIT_PUSH_TRUE_FALSE(jump_opcode)                   \
//    TRY (label_index = get_label_index (compiler));         \
//    EMIT (jump_opcode " push_one_%d", label_index);         \
//    EMIT ("    push 0");                                    \
//    EMIT ("    jmp end_%d", label_index);                   \
//    EMIT ("push_one_%d:", label_index);                     \
//    EMIT ("    push 1");                                    \
//    EMIT ("end_%d:", label_index);
//
//#define EMIT_COMPARATOR(opcode)                             \
//    TRY (subtree_compile (compiler, node->left,  stream));  \
//    TRY (subtree_compile (compiler, node->right, stream));  \
//    EMIT_PUSH_TRUE_FALSE (opcode)
//
//
//static bool compile_op (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (node     != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//
//    assert (node->type == tree::node_type_t::OP);
//
//    char buf[BUF_SIZE]          = "";
//    char var_code_buf[VAR_BUF_SIZE] = "";
//    int label_index    = -1;
//    EMIT (" ");
//
//    switch ((tree::op_t) node->data)
//    {
//        case tree::op_t::ADD: EMIT_BINARY_OP ("add"); break;
//        case tree::op_t::SUB: EMIT_BINARY_OP ("sub"); break;
//        case tree::op_t::MUL: EMIT_BINARY_OP ("mul"); break;
//        case tree::op_t::DIV: EMIT_BINARY_OP ("div"); break;
//
//        case tree::op_t::SQRT:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("sqrt");
//            break;
//
//        case tree::op_t::SIN:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("sin");
//            break;
//
//        case tree::op_t::COS:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("sin");
//            break;
//
//        case tree::op_t::OUTPUT:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("pop  rax");
//            EMIT ("push rax");
//            EMIT ("push rax");
//            EMIT ("out");
//            break;
//
//        case tree::op_t::ASSIG:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("pop  rax");
//            EMIT ("push rax");
//            EMIT ("push rax");
//            TRY (get_var_code (compiler, node->left->data, var_code_buf));
//            EMIT ("pop %s ; Assig", var_code_buf);
//            break;
//
//        case tree::op_t::INPUT:
//            EMIT ("inp");
//            break;
//
//        case tree::op_t::EQ:  EMIT_COMPARATOR ("je" ); break;
//        case tree::op_t::GT:  EMIT_COMPARATOR ("ja" ); break;
//        case tree::op_t::LT:  EMIT_COMPARATOR ("jb" ); break;
//        case tree::op_t::GE:  EMIT_COMPARATOR ("jae"); break;
//        case tree::op_t::LE:  EMIT_COMPARATOR ("jbe"); break;
//        case tree::op_t::NEQ: EMIT_COMPARATOR ("jne"); break;
//
//        case tree::op_t::NOT:
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("push 0")
//            EMIT_PUSH_TRUE_FALSE ("je");
//            break;
//
//        case tree::op_t::AND:
//            TRY (subtree_compile (compiler, node->left, stream));
//            EMIT ("push 0")
//            EMIT_PUSH_TRUE_FALSE ("jne");
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("push 0")
//            EMIT_PUSH_TRUE_FALSE ("jne");
//            EMIT_PUSH_TRUE_FALSE ("je");
//            break;
//
//        case tree::op_t::OR:
//            TRY (subtree_compile (compiler, node->left, stream));
//            EMIT ("push 0")
//            EMIT_PUSH_TRUE_FALSE ("jne");
//            TRY (subtree_compile (compiler, node->right, stream));
//            EMIT ("push 0")
//            EMIT_PUSH_TRUE_FALSE ("jne");
//            EMIT ("add");
//            break;
//
//        default:
//            assert (0 && "Unexpected op type");
//    }
//
//    return true;
//}
//
//#undef EMIT_BINARY_OP
//
//// -------------------------------------------------------------------------------------------------
//static bool compile_if (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (node     != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//    assert (node->type == tree::node_type_t::IF && "Invalid call");
//
//    char buf[BUF_SIZE] = "";
//    int label_index = get_label_index (compiler);
//
//    TRY (subtree_compile (compiler, node->left, stream));
//    EMIT ("push 0");
//
//    if (node->right->left != nullptr)
//    {
//        EMIT ("je else_%d", label_index);
//        TRY (subtree_compile (compiler, node->right->left,  stream));
//        EMIT ("jmp if_end_%d", label_index);
//        EMIT ("else_%d:", label_index);
//        TRY (subtree_compile (compiler, node->right->right, stream));
//        EMIT ("if_end_%d:", label_index);
//    }
//    else
//    {
//        EMIT ("je if_end_%d", label_index);
//        TRY (subtree_compile (compiler, node->right->right, stream));
//        EMIT ("if_end_%d:",   label_index);
//    }
//
//    return true;
//}

// -------------------------------------------------------------------------------------------------

//static bool compile_while (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (node     != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//    assert (node->type == tree::node_type_t::WHILE && "Invalid call");
//
//    char buf[BUF_SIZE] = "";
//    int label_index = get_label_index (compiler);
//
//    EMIT ("while_beg_%d:", label_index);
//
//    TRY (subtree_compile (compiler, node->left, stream));
//
//    EMIT ("push 0");
//    EMIT ("je while_end_%d",  label_index);
//
//    TRY (subtree_compile (compiler, node->right, stream));
//
//    EMIT ("jmp while_beg_%d", label_index);
//    EMIT ("while_end_%d:",    label_index);
//
//    return true;
//}

// -------------------------------------------------------------------------------------------------

//static bool compile_func_def (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (node     != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//    assert (node->type == tree::node_type_t::FUNC_DEF && "Invalid call");
//    char buf[BUF_SIZE] = "";
//
//    compiler->global_frame_size_store = compiler->frame_size;
//    compiler->frame_size = 0;
//    compiler->in_func = true;
//
//    EMIT ("jmp func_%d_def_end", node->data);
//    EMIT ("func_%d:", node->data);
//
//    compile_func_def_args (compiler, node->left, stream);
//    TRY (subtree_compile (compiler, node->right, stream));
//
//    EMIT ("func_%d_def_end:", node->data);
//    EMIT ("; ---FUNC END---")
//    EMIT ("  ");
//
//    compiler->in_func = false;
//    clear_local_vars (compiler);
//    compiler->frame_size = compiler->global_frame_size_store;
//
//    return true;
//}

// -------------------------------------------------------------------------------------------------

//static int compile_func_def_args (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//
//    if (node == nullptr) {
//        return 0;
//    }
//
//    int num_of_args = 0;
//
//    if (node->type == tree::node_type_t::FICTIOUS) {
//        if (node->left != nullptr) {
//            num_of_args += compile_func_def_args (compiler, node->left, stream);
//        }
//
//        if (node->right != nullptr) {
//            num_of_args += compile_func_def_args (compiler, node->right, stream);
//        }
//    }
//    else if (node->type == tree::node_type_t::VAR)
//    {
//        register_var (compiler, node->data);
//        num_of_args = 1;
//    }
//    else
//    {
//        assert (0 && "Broken func def params subtree");
//    }
//
//    return num_of_args;
//}

// -------------------------------------------------------------------------------------------------
//

static bool compile_func_call (compiler_t *compiler, tree::node_t *node, FILE *stream)
{
    assert (compiler != nullptr && "invalid pointer");
    assert (node     != nullptr && "invalid pointer");
    assert (stream   != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::FUNC_CALL && "Invalid call");
    char buf[BUF_SIZE] = "";

    int arg_counter = compile_func_call_args (compiler, node->right, stream);

    EMIT ("push rdx")
    EMIT ("push %d", compiler->frame_size);
    EMIT ("add")
    EMIT ("pop rdx; Increment frame")

    for (int i = arg_counter - 1; i >= 0; --i)
    {
        EMIT ("pop [rdx+%d] ; Fill args", i);
    }

    EMIT ("call func_%d", node->data);

    EMIT ("push rdx")
    EMIT ("push %d", compiler->frame_size);
    EMIT ("sub")
    EMIT ("pop rdx ;Decrement frame")

    return true;
}

// -------------------------------------------------------------------------------------------------

//static int compile_func_call_args (compiler_t *compiler, tree::node_t *node, FILE *stream)
//{
//    assert (compiler != nullptr && "invalid pointer");
//    assert (stream   != nullptr && "invalid pointer");
//
//    int args_counter = 0;
//
//    if (node == nullptr) {
//        return args_counter;
//    }
//
//    if (node->type != tree::node_type_t::FICTIOUS) {
//        args_counter++;
//        subtree_compile (compiler, node, stream);
//        return args_counter;
//    }
//
//    if (node->left != nullptr)
//    {
//        if (node->left->type == tree::node_type_t::FICTIOUS) {
//            args_counter += compile_func_call_args (compiler, node->left, stream);
//        } else {
//            args_counter++;
//            subtree_compile (compiler, node->left, stream);
//        }
//    }
//
//    if (node->right != nullptr)
//    {
//        if (node->right->type == tree::node_type_t::FICTIOUS) {
//            args_counter += compile_func_call_args (compiler, node->right, stream);
//        } else {
//            args_counter++;
//            subtree_compile (compiler, node->right, stream);
//        }
//    }
//
//    return args_counter;
//}

// -------------------------------------------------------------------------------------------------

//static int ctor_vars (vars_t *vars)
//{
//    assert (vars != nullptr && "invalid pointer");
//
//    vars->name_indexes = (int *) calloc (DEFAULT_VARS_CAPACITY, sizeof (int));
//    if (vars->name_indexes == nullptr) { return ERROR; }
//
//    vars->size     = 0;
//    vars->capacity = DEFAULT_VARS_CAPACITY;
//
//    return 0;
//}
//
//static void dtor_vars (vars_t *vars)
//{
//    assert (vars != nullptr && "invalis pointer");
//
//    free (vars->name_indexes);
//
//    #ifndef NDEBUG
//        vars->size     = 0;
//        vars->capacity = 0;
//    #endif
//}

// -------------------------------------------------------------------------------------------------

//static void register_var (compiler_t *compiler, int number)
//{
//    assert (compiler != nullptr && "invalid pointer");
//
//    vars_t *vars = nullptr;
//
//    if (compiler->in_func) {
//        vars = &compiler->local_vars;
//    } else {
//        vars = &compiler->global_vars;
//    }
//
//    if (vars->size == vars->capacity)
//    {
//        vars->name_indexes = (int *) realloc (vars->name_indexes, 2 * vars->capacity * sizeof (int));
//        vars->capacity    *= 2;
//    }
//
//    vars->name_indexes[vars->size] = number;
//    vars->size++;
//
//    compiler->frame_size++;
//}
//
//// -------------------------------------------------------------------------------------------------
//
//static bool get_var_code (compiler_t *compiler, int number, char* code)
//{
//    assert (compiler != nullptr && "Invalid pointer");
//    assert (code     != nullptr && "Invalid pointer");
//
//    for (unsigned int i = 0; i < compiler->local_vars.size; ++i)
//    {
//        if (compiler->local_vars.name_indexes[i] == number)
//        {
//            sprintf (code, "[rdx+%u]", i);
//            return true;
//        }
//    }
//
//    for (unsigned int i = 0; i < compiler->global_vars.size; ++i)
//    {
//        if (compiler->global_vars.name_indexes[i] == number)
//        {
//            sprintf (code, "[%u]", i);
//            return true;
//        }
//    }
//
//    log (ERROR, "FAILED to get var code %d", number);
//
//    return false;
//}
//
//// -------------------------------------------------------------------------------------------------
//
//static void clear_local_vars (compiler_t *compiler)
//{
//    assert (compiler != nullptr && "invalid pointer");
//
//    compiler->local_vars.size = 0;
//}
//
//// -------------------------------------------------------------------------------------------------
//
//static int get_label_index (compiler_t *compiler)
//{
//    assert (compiler != nullptr && "invalid pointer");
//
//    return compiler->cur_label_index++;
//}
