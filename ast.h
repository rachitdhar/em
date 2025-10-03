//
// ast.h
//


#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "llvm.h"



inline Data_Type type_map(const std::string& type)
{
    if (type == "void")   return T_VOID;
    if (type == "bool")   return T_BOOL;
    if (type == "int")    return T_INT;
    if (type == "float")  return T_FLOAT;
    if (type == "char")   return T_CHAR;
    if (type == "string") return T_STRING;

    return T_VOID;
}


enum Expression_Type {
    EXPR_IDENT,
    EXPR_LITERAL,
    EXPR_FUNC_DEF,
    EXPR_IF,
    EXPR_FOR,
    EXPR_WHILE,
    EXPR_DECL,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_FUNC_CALL,
    EXPR_RETURN,
    EXPR_JUMP,
    EXPR_BLOCK
};

enum Jump_Type {
    J_BREAK,
    J_CONTINUE
};


struct AST_Expression {
    Expression_Type expr_type = EXPR_IDENT;

    // set the expression type for a derived struct
    AST_Expression(Expression_Type type): expr_type(type) {}
    AST_Expression(): expr_type(EXPR_IDENT) {}

    // to ensure that in case a derived struct is deleted,
    // the derived struct's destructor is called
    virtual ~AST_Expression() = default;

    virtual llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) = 0;
};


struct Function_Parameter {
    std::string name;
    Data_Type type;
};


struct AST_Identifier : AST_Expression {
    AST_Identifier(): AST_Expression(EXPR_IDENT) {}

    std::string name;

    llvm::Value *generate_ir_pointer(); // for handling lvalues

    llvm::Value *generate_ir(
    llvm::LLVMContext& _context,
    llvm::IRBuilder<> *_builder,
    llvm::Module *_module
    ) override;
};


struct AST_Literal : AST_Expression {
    AST_Literal(): AST_Expression(EXPR_LITERAL) {}

    Data_Type type;

    union {
	bool b;
	int i;
	float f;
	char c;
	std::string *s; // storing the address of the string
    } value;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Function_Definition : AST_Expression {
    AST_Function_Definition(): AST_Expression(EXPR_FUNC_DEF) {}

    Data_Type return_type;
    std::string function_name;
    std::vector<Function_Parameter*> params;
    std::vector<AST_Expression*> block;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_If_Expression : AST_Expression {
    AST_If_Expression(): AST_Expression(EXPR_IF) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression*> block;
    std::vector<AST_Expression*> else_block;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_For_Expression : AST_Expression {
    AST_For_Expression(): AST_Expression(EXPR_FOR) {}

    AST_Expression *init = NULL;
    AST_Expression *condition = NULL;
    AST_Expression *increment = NULL;
    std::vector<AST_Expression*> block;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_While_Expression : AST_Expression {
    AST_While_Expression(): AST_Expression(EXPR_WHILE) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression*> block;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Declaration : AST_Expression {
    AST_Declaration(): AST_Expression(EXPR_DECL) {}

    Data_Type data_type;
    std::string variable_name;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Unary_Expression : AST_Expression {
    AST_Unary_Expression(): AST_Expression(EXPR_UNARY) {}

    bool is_postfix = false;
    Token_Type op = TOKEN_NONE;
    AST_Expression *expr = NULL;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Binary_Expression : AST_Expression {
    AST_Binary_Expression(): AST_Expression(EXPR_BINARY) {}

    Token_Type op = TOKEN_NONE;
    AST_Expression *left = NULL;
    AST_Expression *right = NULL;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Function_Call : AST_Expression {
    AST_Function_Call(): AST_Expression(EXPR_FUNC_CALL) {}

    std::string function_name;
    std::vector<AST_Expression*> params;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Return_Expression : AST_Expression {
    AST_Return_Expression(): AST_Expression(EXPR_RETURN) {}

    AST_Expression *value = NULL;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


struct AST_Jump_Expression : AST_Expression {
    AST_Jump_Expression(): AST_Expression(EXPR_JUMP) {}

    Jump_Type jump_type;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};


// for scoped expression blocks
// (other than the ones attached to if/for/while/functions...)
struct AST_Block_Expression : AST_Expression {
    AST_Block_Expression(): AST_Expression(EXPR_BLOCK) {}

    std::vector<AST_Expression*> block;

    llvm::Value *generate_ir(llvm::LLVMContext& _context, llvm::IRBuilder<> *_builder, llvm::Module *_module) override;
};

#endif
