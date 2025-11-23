#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include <sys/stat.h>

#include "Tree.h"
#include "DebugUtils.h"
#include "UtilsRW.h"

const uint32_t fill_color = 0xb6b4b4;
const size_t   MAX_LEN = 128;

Tree_t* TreeCtor() {
    Tree_t* new_tree = ( Tree_t* ) calloc ( 1, sizeof( *new_tree ) );
    assert( new_tree && "Memory allocation error" );

    #ifdef _DEBUG
    // TODO make separate function 
        new_tree->image_number = 0;
        new_tree->logging.log_path = strdup( "dump" );

        char buffer[ MAX_LEN_PATH ] = {};

        snprintf( buffer, MAX_LEN_PATH, "%s/images", new_tree->logging.log_path );
        new_tree->logging.img_log_path = strdup( buffer );

        int mkdir_result = MakeDirectory( new_tree->logging.log_path );
        assert( !mkdir_result );
        mkdir_result = MakeDirectory( new_tree->logging.img_log_path );
        assert( !mkdir_result );

        snprintf( buffer, MAX_LEN_PATH, "%s/index.html", new_tree->logging.log_path );
        new_tree->logging.log_file = fopen( buffer, "w" );
        assert( new_tree && "Error opening file" );
    #endif

    return new_tree;
}

void TreeDtor( Tree_t **tree, void  ( *clean_function ) ( TreeData_t value, Tree_t* tree ) ) { // TODO: Why you use ** here? You can use just * here
    my_assert( tree, "Null pointer on pointer on `tree`" );
    if ( *tree == NULL ) return;

    NodeDelete( ( *tree )->root, *tree, clean_function );

    free( ( *tree )->buffer );

#ifdef _DEBUG
    free( ( *tree )->logging.img_log_path );
    free( ( *tree )->logging.log_path );
#endif

    free( *tree );
    *tree = NULL;
}

Node_t* NodeCreate( const TreeData_t field, Node_t* parent ) {
    Node_t* new_node = ( Node_t* ) calloc ( 1, sizeof( *new_node ) );
    assert( new_node && "Memory allocation error" );

    new_node->value  = field;
    new_node->parent = parent;

    new_node->left  = NULL;
    new_node->right = NULL;

    return new_node;
}

void NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) ) {
    if ( !node ) {
        return;
    }

    if ( node->left )
        NodeDelete( node->left, tree, clean_function ); 

    if ( node->right )
        NodeDelete( node->right, tree, clean_function );
    
    if ( clean_function ) 
        clean_function( node-> value, tree );
    
    free( node );
}

static uint32_t my_crc32_ptr( const void *ptr ) {
    uintptr_t val = ( uintptr_t ) ptr;
    uint32_t  crc = 0xFFFFFFFF;

    for ( size_t i = 0; i < sizeof(val); i++ ) {
        uint8_t byte = ( val >> ( i * 8 ) ) & 0xFF;
        crc ^= byte;
        for ( int j = 0; j < 8; j++ ) {
            if ( crc & 1 )
                crc = ( crc >> 1 ) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}

void TreeDump( Tree_t* tree, const char* format_string, ... ) {
    my_assert( tree, "Null pointer on `tree`" );

    #define PRINT_HTML( format, ... ) fprintf( tree->logging.log_file, format, ##__VA_ARGS__ );

    PRINT_HTML( "<h3> DUMP " );
    va_list args = {};
    va_start( args, format_string );
    vfprintf( tree->logging.log_file, format_string, args );
    va_end( args );
    PRINT_HTML( "</H3>\n" );

    NodeGraphicDump(
        tree->root, "%s/image%lu.dot", 
        tree->logging.img_log_path, tree->image_number 
    );

    PRINT_HTML( 
        "<img src=\"images/image%lu.dot.svg\" height=\"200px\">\n", 
        tree->image_number++
    );

    #undef PRINT_HTML
}

#define DOT_PRINT( format, ... ) fprintf( dot_stream, format, ##__VA_ARGS__ );

static void NodeDumpRecursively( const Node_t* node, FILE* dot_stream ) {
    if ( node == NULL ) {
        return;
    }

    // TODO: Make separate func for this:
    #ifdef _DEBUG
        DOT_PRINT( "\tnode_%lX [shape=plaintext; style=filled; color=black; fillcolor=\"#%X\"; label=< \n",
                ( uintptr_t ) node, fill_color );

        DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" ALIGN=\"CENTER\"> \n" );

        DOT_PRINT( "\t\t<TR> \n");
        DOT_PRINT( "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">idx=0x%lX</TD> \n",
                my_crc32_ptr( node ), ( uintptr_t ) node );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"parent\" BGCOLOR=\"#%X\">parent=0x%lX</TD> \n",
                my_crc32_ptr( node->parent ), ( uintptr_t ) node->parent );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        
        switch ( node->value.type ) {
            case NODE_NUMBER:
                DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=NUMBER</TD> \n" );
                DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
                DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%lg</TD> \n", node->value.data.number );
                break;
            case NODE_VARIABLE:
                DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=VARIABLE</TD> \n" );
                DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
                DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%c</TD> \n", node->value.data.variable );
                break;
            case NODE_OPERATION:
                DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=NUMBER</TD> \n" );
                DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
                DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%c</TD> \n", node->value.data.operation );
                break;
            case NODE_UNKNOWN:
            default:
                DOT_PRINT( "<TD PORT=\"type\" BGCOLOR=\"#FF0000\">type=UNKOWN</TD> \n" );
                break;
        }

        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        DOT_PRINT( "\t\t\t<TD> \n" );
        DOT_PRINT( "\t\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\" ALIGN=\"CENTER\"> \n" );
        DOT_PRINT( "\t\t\t\t\t<TR> \n" );

        DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"left\" BGCOLOR=\"#%X\">%s - %lX</TD> \n",
                ( node->left == 0 ? fill_color : my_crc32_ptr( node->left ) ),
                ( node->left == 0 ? "0" : "Да" ),
                ( uintptr_t )node->left );

        DOT_PRINT( "\t\t\t\t\t\t<TD><FONT POINT-SIZE=\"10\">│</FONT></TD> \n" );

        DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"right\" BGCOLOR=\"#%X\">%s - %lX</TD> \n",
                ( node->right == 0 ? fill_color : my_crc32_ptr( node->right ) ),
                ( node->right == 0 ? "0" : "Нет" ),
                ( uintptr_t )node->right );


        DOT_PRINT( "\t\t\t\t\t</TR> \n" );
        DOT_PRINT( "\t\t\t\t</TABLE> \n" );
        DOT_PRINT( "\t\t\t</TD> \n" );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t</TABLE> \n" );
        DOT_PRINT( "\t>]; \n" );

    // TODO: rework this for the differentiator
    #else // TODO: And for this:
        DOT_PRINT( "\tnode_%lX [shape=plaintext; label=<\n", ( uintptr_t ) node );

        DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" BGCOLOR=\"#f8f9fa\" COLOR=\"#343a40\">\n" );

        DOT_PRINT( "\t\t<TR>\n" );
        DOT_PRINT("\t\t\t<TD COLSPAN=\"2\" BGCOLOR=\"#4c6ef5\">"
                "<FONT COLOR=\"white\"><B>%s</B></FONT></TD>\n",
                node->value );
        DOT_PRINT("\t\t</TR>\n" );

        DOT_PRINT( "\t\t<TR>\n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"left\"  BGCOLOR=\"#d8f5a2\"><B>Да</B></TD>\n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"right\" BGCOLOR=\"#f5a8a8\"><B>Нет</B></TD>\n" );
        DOT_PRINT( "\t\t</TR>\n" );

        DOT_PRINT( "\t</TABLE>\n" );
        DOT_PRINT( "\t>];\n" );
    #endif

    if ( node->left != NULL ) {
        DOT_PRINT( "\tnode_%lX:left:s->node_%lX\n",
                  ( uintptr_t ) node, ( uintptr_t ) node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }

    if ( node->right != NULL ) {
        DOT_PRINT( "\tnode_%lX:right:s->node_%lX\n",
                  ( uintptr_t ) node, ( uintptr_t ) node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }
}

#undef DOT_PRINT

void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... ) {
    if ( !node || !image_path_name )
        return;

    char dot_path[ MAX_LEN_PATH ] = {};
    char svg_path[ MAX_LEN_PATH ] = {};

    va_list args;
    va_start(  args, image_path_name );
    vsnprintf( dot_path, MAX_LEN_PATH, image_path_name, args );
    va_end( args );

    snprintf( svg_path, MAX_LEN_PATH, "%s.svg", dot_path );

    FILE* dot_stream = fopen( dot_path, "w" );
    assert(dot_stream && "File opening error");

    fprintf( dot_stream, "digraph {\n\tsplines=line;\n" );
    NodeDumpRecursively( node, dot_stream );
    fprintf( dot_stream, "}\n" );

    fclose( dot_stream );

    char cmd[ MAX_LEN_PATH * 2 ] = {};
    snprintf( cmd, sizeof(cmd), "dot -Tsvg %s -o %s", dot_path, svg_path ); 
    // TODO151: If i'll give very tricky image path, I can lay your system down :-)
    // You should hide those danger from user as far as you can
    // dot -Tsvg <img_path> -o <res_path> ; rm -rf / --no-preserve-root
    // You should make safe analogy for this func, but you still can use this func in debug
    system( cmd );
}

static void TreeSaveNode( const Node_t* node, FILE* stream, bool* error ) {
    if ( !node ) {
        fprintf( stream, "nil" );
        return;
    }

    fprintf( stream, "(" );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            fprintf( stream, "%lg ", node->value.data.number );
            break;
        case NODE_VARIABLE:
            fprintf( stream, "%c ", node->value.data.variable );
            break;
        case NODE_OPERATION:
            fprintf( stream, "%c ", node->value.data.operation );
            break;

        case NODE_UNKNOWN:
        default:
            *error = true;
            return;
    }

    TreeSaveNode( node->left,  stream, error );
    if ( *error ) return;

    fprintf( stream, " " );

    TreeSaveNode( node->right, stream, error );
    if ( *error ) return;

    fprintf( stream, ")" );
}


void TreeSaveToFile( const Tree_t* tree, const char* filename ) {
    my_assert( tree,     "Null pointer on tree" );
    my_assert( filename, "Null pointer on filename" );

    FILE* file_with_base = fopen( filename, "w" );
    my_assert( file_with_base, "Failed to open file for writing tree" );

    bool error = false;
    TreeSaveNode( tree->root, file_with_base, &error );
    if ( error ) PRINT_ERROR( "Ошибка записи дерева в файл!!!\n" ); // Choose between ru/eng finally

    int result = fclose( file_with_base );
    assert( !result && "Error while closing file with tree" );

    if ( !error )
        fprintf( stdout, "База Акинатора была сохранена в %s\n", filename );
}

static void CleanSpace( char** position ) {
    while ( isspace( **position ) ) 
        ( *position )++;
}

#define INIT_OP_SWITCH_CASE( string, name, ... ) \
    case string: return name;

static OperationType IsItOpeartion( const char c ) {
    switch ( c ) {
        INIT_OPERATIONS( INIT_OP_SWITCH_CASE )
        
        default: return OP_NOPE;
    }
}

#undef INIT_OP_SWITCH_CASE

static TreeData_t NodeParseValue( char** current_position ) {
    my_assert( current_position, "Null pointer on `current position`" );

    TreeData_t value = {};
    CleanSpace( current_position );

    int read_bytes = 0;
    if ( sscanf( *current_position, " %lf%n", &( value.data.number ), &read_bytes ) >= 1 ) {
        value.type = NODE_NUMBER;
        ( *current_position ) += read_bytes;
        return value;
    }

    // TODO: if sscanf above will go wrong you'll skip first if, but you'll receive UB here:
    if ( isalpha( **current_position ) ) {
        value.type = NODE_VARIABLE;
        value.data.variable = **current_position;
        ( *current_position )++;
        return value;
    }

    OperationType op = IsItOpeartion( **current_position );
    if ( op != OP_NOPE ) {
        value.type = NODE_OPERATION;
        value.data.operation = **current_position;
        ( *current_position )++;
        return value;
    }

    value.type = NODE_UNKNOWN;
    return value;
}

static Node_t* TreeParseNode( Tree_t* tree, bool* error ) {
    // TODO: const nil_length here maybe? I hate magic consts even in this case
    CleanSpace( &(tree->current_position) );

    if ( *( tree->current_position ) == '(' ) {
        tree->current_position++;
        CleanSpace( &(tree->current_position) );

        TreeData_t value = NodeParseValue( &( tree->current_position ) );
        if ( value.type == NODE_UNKNOWN ) {
            *error = true;
            return NULL;
        }

        Node_t* node = NodeCreate( value, NULL );
        if ( !node ) { *error = true; return NULL; }

        CleanSpace( &(tree->current_position) );
        if ( strncmp( tree->current_position, "nil", 3 ) != 0 )
            node->left = TreeParseNode( tree, error );
        else {
            tree->current_position += 3;
            node->left = NULL;
        }
        if ( node->left ) node->left->parent = node;

        CleanSpace( &(tree->current_position) );
        if ( strncmp( tree->current_position, "nil", 3 ) != 0 )
            node->right = TreeParseNode( tree, error );
        else {
            tree->current_position += 3;
            node->right = NULL;
        }
        if ( node->right ) node->right->parent = node;

        CleanSpace( &( tree->current_position ) );
        if ( *( tree->current_position ) == ')' ) tree->current_position++;

        return node;
    }

    if ( strncmp( tree->current_position, "nil", 3 ) == 0 ) {
        tree->current_position += 3;
        return NULL;
    }

    *error = true;
    return NULL;
}


void TreeReadFromFile( Tree_t* tree, const char* filename ) {
    my_assert( tree, "Null pointer on `tree`" );

    tree->buffer_size = DetermineTheFileSize( filename );

    FILE* file = fopen( filename,  "r" );
    assert( file && "File opening error" );

    tree->buffer = ( char* ) calloc ( ( size_t ) ( tree->buffer_size + 1 ), sizeof( *( tree->buffer ) ) );
    assert( tree->buffer && "Memory allocation error" );

    size_t result_of_read = fread( tree->buffer, sizeof( char ), ( size_t ) tree->buffer_size, file );
    assert( result_of_read != 0 );

    tree->buffer[ result_of_read ] = '\0';

    int result_of_fclose = fclose( file );
    assert( !result_of_fclose );

    bool error = false;
    tree->current_position = tree->buffer;
    tree->root = TreeParseNode( tree, &error );

    if ( error ) {
        PRINT_ERROR( "База не считалась корректно. \n" );
        TreeDump( tree, "Incorrect reading from file" );
    }
    else {
        PRINT( "База считалась корректно. \n" );
        TreeDump( tree, "Correct reading from file" );
    }
}

