#include <cmath>
#include <math.h>
#include <string.h>

#include "Expression.h"
#include "DebugUtils.h"

const size_t MAX_SIZE = 256; // TODO // FIXME: Why do you limit variable number? Make dynamic array

struct Variable_t {
    char name;
    double value;
};


static double SearchVariable( char name, Variable_t variables[ MAX_SIZE ], 
                                            size_t* number_of_variables ) { // TODO SearchVariableValue
    my_assert( variables, "Null pointer on `inited_variables" );

    for ( size_t idx = 0; idx < *number_of_variables; idx++ ) {
        if ( name == variables[ idx ].name ) {
            return variables[ idx ].value;
        }
    }

    printf( "Введите значение переменной %c: ", name );

    // TODO: check for incorrect input
    variables[ *number_of_variables ].name = name;
    int scanf_result = scanf( "%lf", &( variables[ *number_of_variables ].value ) );

    return variables[ ( *number_of_variables )++ ].value;
}

#define OPERATIONS( )

static double EvaluateNode( Node_t* node, Variable_t variables[ MAX_SIZE ], size_t number_of_variables ) {
    if ( !node ) {
        return 0;
    }

    switch ( node->value.type ) {
        case NODE_NUMBER:
            return node->value.data.number;
        case NODE_VARIABLE:
            return SearchVariable( node->value.data.variable, variables, &number_of_variables );
        case NODE_OPERATION: {
            char op = node->value.data.operation;

            double L = EvaluateNode( node->left, variables, number_of_variables );
            double R = EvaluateNode( node->right, variables, number_of_variables );

            switch ( op ) {
                case '+': return L + R;
                case '-': return L - R;
                case '*': return L * R;
                case '/': return L / R; // TODO: R = 0 case?

                default:
                    PRINT_ERROR( "Ошибка: неизвестная операция '%c'\n", op );
                    return NAN;
            }
        }
        
        case NODE_UNKNOWN:
        default:
            PRINT_ERROR( "Ошибка: такая нода вообще не должна была здесь появиться!!! \n" );
            return NAN;

    }

    return NAN;
}

double EvaluateTree( const Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    double result = 0;
    Variable_t inited_variables[ MAX_SIZE ] = {};
    size_t size = 0;

    result = EvaluateNode( tree->root, inited_variables, size );

    return result;
}