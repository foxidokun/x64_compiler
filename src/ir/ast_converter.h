#ifndef AST_CONVERTER_H
#define AST_CONVERTER_H

#include "ir.h"
#include "../lib/tree.h"

namespace ir
{
    result_t from_ast(ir::code_t *self, const tree::tree_t *tree);
}

#endif