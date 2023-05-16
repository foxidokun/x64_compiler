#ifndef TREE_H
#define TREE_H

#include <cstdarg>
#include <stdlib.h>
#include <stdio.h>

namespace tree
{
    enum class node_type_t
    {
        NOT_SET  = -1,
        FICTIOUS = 0,
        VAL,
        VAR,
        IF,
        ELSE,
        WHILE,
        OP,
        VAR_DEF,
        FUNC_DEF,
        FUNC_CALL,
        RETURN
    };

    enum class op_t
    {
        ADD = 1,
        SUB,
        MUL,
        DIV,
        SQRT,
        INPUT,
        OUTPUT,
        EQ,     // ==
        GT,     // >
        LT,     // <
        GE,     // >=
        LE,     // <=
        NEQ,    // !=
        NOT,    // !
        AND,    // &&
        OR,     // ||
        ASSIG,   // :=
        SIN = 100,
        COS = 101,
    };

    struct node_t
    {
        node_type_t type = node_type_t::NOT_SET;
        int data;

        node_t *left    = nullptr;
        node_t *right   = nullptr;
    };

    struct tree_t
    {
        node_t *head_node;
    };

    enum tree_err_t
    {
        OK = 0,
        OOM,
        INVALID_DUMP,
        MMAP_FAILURE
    };

    typedef bool (*walk_f)(node_t *node, void *param, bool cont);

    void ctor (tree_t *tree);
    void dtor (tree_t *tree);

    bool dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                 walk_f in_exec,   void *in_param,
                                 walk_f post_exec, void *post_param);
    bool dfs_exec (node_t *node, walk_f pre_exec,  void *pre_param,
                                 walk_f in_exec,   void *in_param,
                                 walk_f post_exec, void *post_param);

    void change_node (node_t *node, node_type_t type, int data);

    void move_node (node_t *dest, node_t *src);

    tree::node_t *copy_subtree (tree::node_t *node);

    void    save_tree (tree_t *tree, FILE *stream);
    void    save_tree (node_t *tree, FILE *stream);
    node_t *load_tree (              const char *content);

    tree::node_t *new_node ();
    tree::node_t *new_node (node_type_t type, int  data);
    tree::node_t *new_node (node_type_t type, op_t op  );
    tree::node_t *new_node (node_type_t type, int  data, node_t *left, node_t *right);
    tree::node_t *new_node (node_type_t type, op_t op  , node_t *left, node_t *right);

    void del_node (node_t *node);

    void del_left   (node_t *node);
    void del_right  (node_t *node);
    void del_childs (node_t *node);
}

#endif //TREE_H
