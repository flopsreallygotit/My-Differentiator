#include <stdio.h> // TODO: All nested include directives should be protected by header guard too. Move it down
/* It works fine just because system headers have it's own guards https://godbolt.org/z/z6TveGY5f */

#ifndef TREE_H
#define TREE_H

#ifdef _LINUX
#include <linux/limits.h>
const size_t MAX_LEN_PATH = PATH_MAX;
#else
const size_t MAX_LEN_PATH = 256;
#endif

#define INIT_OPERATIONS( macros ) \
    macros( '+', OP_ADD, 0 ) \
    macros( '-', OP_SUB, 1 ) \
    macros( '*', OP_MUL, 2 ) \
    macros( '/', OP_DIV, 3 ) \
    macros( '^', OP_POW, 4 )

#define INIT_OP_ENUM( str, name, value ) \
    name = value,

enum NodeType {
    NODE_UNKNOWN = -1,

    NODE_NUMBER    = 0,
    NODE_VARIABLE  = 1,
    NODE_OPERATION = 2
};

enum OperationType {
    OP_NOPE = -1,

    INIT_OPERATIONS( INIT_OP_ENUM )
};

struct NodeValue { // That's cool decision!
    enum NodeType type;

    union {
        double number; 
        char   variable; 
        char   operation; 
    } data;
};

typedef NodeValue TreeData_t;

struct Node_t {
    TreeData_t value;

    Node_t* right;
    Node_t* left;

    Node_t* parent;
};

struct Tree_t {
    Node_t* root;

    /* TODO:
    I think that it's bad idea to store buffer with initial expression
    in Tree struct. You should make Differentiator struct, that will contain:
    Tree_t *tree;
    ExprInfo_t *expr_info;
    e.t.c
    because tree shouldn't provide any functional that implements any interaction with
    initial expression, but differentiator should.
    */

    char* buffer; 
    char* current_position;
    off_t buffer_size;

#ifdef _DEBUG
    struct Log_t {
        FILE* log_file;
        char* log_path;
        char* img_log_path;
    } logging;

    size_t image_number; // TODO151: You can move it to Log_t struct and provide unique log artifact number, that will ease interaction with this part
#endif
};

enum TreeStatus_t { // TODO: Remove if you don't use it
    SUCCESS = 0,
    FAIL    = 1
};

enum DirectionType { // TODO: Remove if you don't use it
    RIGHT = 0,
    LEFT  = 1
};

Tree_t* TreeCtor();
void    TreeDtor( Tree_t** tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeSaveToFile( const Tree_t* tree, const char* filename );
void TreeReadFromFile    ( Tree_t* tree, const char* filename );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
void    NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

void TreeDump( Tree_t* tree, const char* format_string, ... );
void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H