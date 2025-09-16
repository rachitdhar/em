//
// ast.h
//


#ifndef AST_H
#define AST_H

#include "lexer.h"


enum Expression_Type {
    EXPR_IDENT,
    EXPR_LITERAL,
    EXPR_FUNC_DEF,
    EXPR_IF,
    EXPR_FOR,
    EXPR_WHILE,
    EXPR_DECL,
    EXPR_BINARY,
    EXPR_FUNC_CALL,
    EXPR_RETURN,
    EXPR_JUMP
};


struct AST_Expression {
    Expression_Type expr_type = EXPR_IDENT;

    // set the expression type for a derived struct
    AST_Expression(Expression_Type type): expr_type(type) {}
    AST_Expression(): expr_type(EXPR_IDENT) {}

    // to ensure that in case a derived struct is deleted,
    // the derived struct's destructor is called
    virtual ~AST_Expression() = default;
};


struct Function_Parameter {
    std::string name;
    std::string type;
};


struct AST_Identifier : AST_Expression {
    AST_Identifier(): AST_Expression(EXPR_IDENT) {}

    std::string name;
};


struct AST_Literal : AST_Expression {
    AST_Literal(): AST_Expression(EXPR_LITERAL) {}

    union {
	int i;
	float f;
	char c;
	std::string *s; // storing the address of the string
    } value;
};


struct AST_Function_Definition : AST_Expression {
    AST_Function_Definition(): AST_Expression(EXPR_FUNC_DEF) {}

    std::string return_type;
    std::string function_name;
    std::vector<Function_Parameter*> params;
    std::vector<AST_Expression*> block;
};


struct AST_If_Expression : AST_Expression {
    AST_If_Expression(): AST_Expression(EXPR_IF) {}

    AST_Expression *condition;
    std::vector<AST_Expression*> block;
    std::vector<AST_Expression*> else_block;
};


struct AST_For_Expression : AST_Expression {
    AST_For_Expression(): AST_Expression(EXPR_FOR) {}

    AST_Expression *init;
    AST_Expression *condition;
    AST_Expression *increment;
    std::vector<AST_Expression*> block;
};


struct AST_While_Expression : AST_Expression {
    AST_While_Expression(): AST_Expression(EXPR_WHILE) {}

    AST_Expression *condition;
    std::vector<AST_Expression*> block;
};


struct AST_Declaration : AST_Expression {
    AST_Declaration(): AST_Expression(EXPR_DECL) {}

    std::string data_type;
    std::string variable_name;
};


struct AST_Binary_Expression : AST_Expression {
    AST_Binary_Expression(): AST_Expression(EXPR_BINARY) {}

    Token_Type op;
    AST_Expression *left;
    AST_Expression *right;
};


struct AST_Function_Call : AST_Expression {
    AST_Function_Call(): AST_Expression(EXPR_FUNC_CALL) {}

    std::string function_name;
    std::vector<AST_Expression*> params;
};


struct AST_Return_Expression : AST_Expression {
    AST_Return_Expression(): AST_Expression(EXPR_RETURN) {}

    AST_Expression *value;
};


struct AST_Jump_Expression : AST_Expression {
    AST_Jump_Expression(): AST_Expression(EXPR_JUMP) {}

    std::string jump_type;
};

#endif
