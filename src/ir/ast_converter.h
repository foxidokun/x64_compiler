#ifndef AST_CONVERTER_H
#define AST_CONVERTER_H

#include "ir.h"
#include "../lib/tree.h"

namespace ir
{
    ir::code_t *from_ast(tree::node_t *node);
}

#endif