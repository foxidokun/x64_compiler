#include <assert.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "../common.h"
#include "file.h"
#include "log.h"

#include "tree.h"

// -------------------------------------------------------------------------------------------------

struct dfs_params
{
    FILE *stream;
    char ** var_names;
    char **func_names;
};

// -------------------------------------------------------------------------------------------------
// CONST SECTION
// -------------------------------------------------------------------------------------------------

const int REASON_LEN   = 50;
const int MAX_NODE_LEN = 32;

const char PREFIX[] = "digraph G {\nnode [shape=record,style=\"filled\"]\nsplines=spline;\n";
static const size_t DUMP_FILE_PATH_LEN = 20;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

// -------------------------------------------------------------------------------------------------
// STATIC PROTOTYPES SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param);

static tree::node_t *load_subtree (const char **str);

static bool node_codegen   (tree::node_t *node, void *void_params, bool);
static bool middle_codegen (tree::node_t *node, void *void_params, bool);
static bool close_subgraph (tree::node_t *node, void *void_params, bool);
static const char *get_op_name (tree::op_t op);
static void format_node (const tree::node_t *node, char *buf, char **var_names, char **func_names, const char **color);


// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

void tree::ctor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    tree->head_node = nullptr;
}

void tree::dtor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    del_node (tree->head_node);
}

// -------------------------------------------------------------------------------------------------

bool tree::dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (tree != nullptr && "invalid pointer");
    assert (tree->head_node != nullptr && "invalid tree");

    return dfs_recursion (tree->head_node, pre_exec,  pre_param,
                                           in_exec,   in_param,
                                           post_exec, post_param);
}

bool tree::dfs_exec (node_t *node, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    return dfs_recursion (node, pre_exec,  pre_param,
                                in_exec,   in_param,
                                post_exec, post_param);
}

// -------------------------------------------------------------------------------------------------

void tree::change_node (node_t *node, node_type_t type, int data)
{
    assert (node != nullptr && "invalid pointer");

    node->data = data;
    node->type = type;
}

// -------------------------------------------------------------------------------------------------

void tree::move_node (node_t *dest, node_t *src)
{
    assert (dest != nullptr && "invalid pointer");
    assert (src  != nullptr && "invalid pointer");

    memcpy (dest, src, sizeof (node_t));

    free (src);
}

// -------------------------------------------------------------------------------------------------

#define NEW_NODE_IN_CASE(type, field)               \
    case tree::node_type_t::type:                   \
        node_copy = tree::new_node (node->field);   \
        if (node_copy == nullptr) return nullptr;   \
        break;

tree::node_t *tree::copy_subtree (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    tree::node_t *node_copy = new_node (node->type, node->data);

    if (node->right != nullptr) {
        node_copy->right = copy_subtree (node->right);
    } else {
        node_copy->right = nullptr;
    }

    if (node->left != nullptr) {
        node_copy->left = copy_subtree (node->left);
    } else {
        node_copy->left = nullptr;
    }

    return node_copy;
}

#undef NEW_NODE_IN_CASE

// -------------------------------------------------------------------------------------------------

int tree::graph_dump (tree_t *tree, const char *reason_fmt, char **var_names, char **func_names, ...)
{
    assert (tree       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, func_names);

    int res = graph_dump (tree->head_node, reason_fmt, var_names, func_names, args);

    va_end (args);
    return res;
}

int tree::graph_dump (tree_t *tree, const char *reason_fmt, ...)
{
    assert (tree       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (tree->head_node, reason_fmt, args);

    va_end (args);
    return res;
}

int tree::graph_dump (node_t *node, const char *reason_fmt, char **var_names, char **func_names, ...)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, func_names);

    int res = graph_dump (node, reason_fmt, var_names, func_names, args);

    va_end (args);
    return res;
}


int tree::graph_dump (node_t *node, const char *reason_fmt, ...)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (node, reason_fmt, args);

    va_end (args);
    return res;
}

int tree::graph_dump (node_t *node, const char *reason_fmt, va_list args)
{
    return graph_dump (node, reason_fmt, nullptr, nullptr, args);
}

int tree::graph_dump (node_t *node, const char *reason_fmt, char **var_names, char **func_names, va_list args)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");


    static int counter = 0;
    counter++;

    char filepath[DUMP_FILE_PATH_LEN+1] = "";    
    sprintf (filepath, DUMP_FILE_PATH_FORMAT, counter);

    FILE *dump_file = fopen (filepath, "w");
    if (dump_file == nullptr)
    {
        log (ERROR, "Failed to open dump file '%s'", filepath);
        return counter;
    }

    fprintf (dump_file, PREFIX);
    
    dfs_params params = {dump_file, var_names, func_names};

    dfs_exec (node, node_codegen,    &params,
                    middle_codegen,    &params,
                    close_subgraph,  &params);

    fprintf (dump_file, "}\n");

    fclose (dump_file);

    char cmd[2*DUMP_FILE_PATH_LEN+20+1] = "";
    sprintf (cmd, "dot -T png -o %s.png %s", filepath, filepath);
    if (system (cmd) != 0)
    {
        log (ERROR, "Failed to execute '%s'", cmd);
    }

    #if HTML_LOGS
        FILE *stream = get_log_stream ();

        fprintf  (stream, "<h2>List dump: ");
        vfprintf (stream, reason_fmt, args);
        fprintf  (stream, "</h2>");

        fprintf (stream, "\n\n<img src=\"%s.png\">\n\n", filepath);
    #else
        char buf[REASON_LEN] = "";
        vsprintf (buf, reason_fmt, args);
        log (INFO, "Dump path: %s.png, reason: %s", filepath, buf);
    #endif

    fflush (get_log_stream ());
    return counter;
}

// -------------------------------------------------------------------------------------------------

void tree::save_tree (tree_t *tree, FILE *stream)
{
    assert (tree   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    save_tree (tree->head_node, stream);
}

// -------------------------------------------------------------------------------------------------

tree::node_t *tree::load_tree (const char *str)
{
    return load_subtree (&str);
}

// -------------------------------------------------------------------------------------------------

tree::node_t *tree::new_node ()
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    const tree::node_t default_node = {};
    memcpy (node, &default_node, sizeof (tree::node_t));

    node->type    = node_type_t::NOT_SET;   

    return node;
}

tree::node_t *tree::new_node (node_type_t type, int data)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = type;
    node->data = data;    

    return node;
}

tree::node_t *tree::new_node (node_type_t type, op_t op)
{
    return new_node(type, (int) op);
}

tree::node_t *tree::new_node (node_type_t type, int data, node_t *left, node_t *right)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type  = type;
    node->data  = data;   
    node->left  = left;
    node->right = right; 

    return node;
}

tree::node_t *tree::new_node (node_type_t type, op_t op, node_t *left, node_t *right)
{
    return new_node(type, (int) op, left, right);
}

// -------------------------------------------------------------------------------------------------

void tree::del_node (node_t *start_node)
{
    if (start_node == nullptr)
    {
        return;
    }

    tree::walk_f free_node_func = [](node_t* node, void *, bool){ free(node); return true; };

    dfs_recursion (start_node, nullptr,        nullptr,
                               nullptr,        nullptr,
                               free_node_func, nullptr);
}

void tree::del_left  (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->left);
    node->left = nullptr;
}

void tree::del_right (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    node->right = nullptr;
}

void tree::del_childs (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    del_node (node->left);
    node->right = nullptr;
    node->left  = nullptr;
}

// -------------------------------------------------------------------------------------------------
// PRIVATE SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    bool cont = true; 

    if (pre_exec != nullptr)
    {
        cont = pre_exec (node, pre_param, cont) && cont;
    }

    if (cont && node->left != nullptr)
    {
        cont = cont && dfs_recursion (node->left, pre_exec,  pre_param,
                                                  in_exec,   in_param,
                                                  post_exec, post_param);
    }

    if (in_exec != nullptr)
    {
        cont = in_exec (node, in_param, cont) && cont;

    }

    if (cont && node->right != nullptr)
    {
        cont = cont && dfs_recursion (node->right, pre_exec,  pre_param,
                                    in_exec,   in_param,
                                    post_exec, post_param);
    }

    if (post_exec != nullptr)
    {
        cont = post_exec (node, post_param, cont) && cont;
    }

    return cont;
}

// -------------------------------------------------------------------------------------------------

#define SKIP_SPACES() while (isspace (**str)) { (*str)++; };
#define CHECK(ch)     if (**str != ch) { return nullptr; } else {(*str)++;}
#define TRY(expr)     if ((expr) == nullptr) { return nullptr; };

static tree::node_t *load_subtree (const char **str)
{
    assert ( str != nullptr && "invalid pointer");
    assert (*str != nullptr && "invalid pointer");

    int type = -1;
    int data = -1;
    int nchars = 0;
    tree::node_t *first  = nullptr;
    tree::node_t *second = nullptr;

    SKIP_SPACES ();
    CHECK ('{');
    sscanf (*str, "%d%d%n", &type, &data, &nchars);
    *str += nchars;

    SKIP_SPACES ();
    if (**str == '{')
    {
        TRY (first = load_subtree (str));
    }

    SKIP_SPACES ();
    if (**str == '{')
    {
        TRY (second = load_subtree (str));
    }

    SKIP_SPACES();
    CHECK('}');

    if (second != nullptr) {
        return tree::new_node ((tree::node_type_t) type, data, first, second);
    } else if (first != nullptr) {
        return tree::new_node ((tree::node_type_t) type, data, nullptr, first);
    } else {
        return tree::new_node ((tree::node_type_t) type, data);
    } 
}

#undef SKIP_SPACES
#undef CHECK  
#undef TRY
