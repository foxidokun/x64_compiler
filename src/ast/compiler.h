#ifndef COMPILER_H
#define COMPILER_H

#include "../lib/tree.h"

struct vars_t {
    int *name_indexes;

    unsigned int size;
    unsigned int capacity;
};

//TODO понадобится стек размеров фрейма
struct compiler_t
{
    vars_t global_vars;
    vars_t local_vars;

    bool in_func;
    int global_frame_size_store;
    int frame_size;

    int cur_label_index;
};

namespace compiler
{
    void ctor (compiler_t *compiler);
    void dtor (compiler_t *compiler);

    bool compile (tree::node_t *node, FILE *stream);
}

#endif