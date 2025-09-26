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


inline llvm::Type *llvm_type_map(const Data_Type type)
{
    switch (type) {
        case T_INT:    return llvm::Type::getInt32Ty(_context);
        case T_FLOAT:  return llvm::Type::getFloatTy(_context);
        case T_BOOL:   return llvm::Type::getInt1Ty(_context);
        case T_CHAR:   return llvm::Type::getInt8Ty(_context);
        case T_STRING: return llvm::Type::getInt8PtrTy(_context);
        case T_VOID:   return llvm::Type::getVoidTy(_context);
        default:
            return nullptr;
    }
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


struct AST_Expression {
    Expression_Type expr_type = EXPR_IDENT;

    // set the expression type for a derived struct
    AST_Expression(Expression_Type type): expr_type(type) {}
    AST_Expression(): expr_type(EXPR_IDENT) {}

    // to ensure that in case a derived struct is deleted,
    // the derived struct's destructor is called
    virtual ~AST_Expression() = default;

    virtual llvm::Value* codegen() = 0;
};


struct Function_Parameter {
    std::string name;
    Data_Type type;
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

    Data_Type return_type;
    std::string function_name;
    std::vector<Function_Parameter*> params;
    std::vector<AST_Expression*> block;
};


struct AST_If_Expression : AST_Expression {
    AST_If_Expression(): AST_Expression(EXPR_IF) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression*> block;
    std::vector<AST_Expression*> else_block;
};


struct AST_For_Expression : AST_Expression {
    AST_For_Expression(): AST_Expression(EXPR_FOR) {}

    AST_Expression *init = NULL;
    AST_Expression *condition = NULL;
    AST_Expression *increment = NULL;
    std::vector<AST_Expression*> block;
};


struct AST_While_Expression : AST_Expression {
    AST_While_Expression(): AST_Expression(EXPR_WHILE) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression*> block;
};


struct AST_Declaration : AST_Expression {
    AST_Declaration(): AST_Expression(EXPR_DECL) {}

    Data_Type data_type;
    std::string variable_name;
};


struct AST_Unary_Expression : AST_Expression {
    AST_Unary_Expression(): AST_Expression(EXPR_UNARY) {}

    bool is_postfix = false;
    Token_Type op = TOKEN_NONE;
    AST_Expression *expr = NULL;
};


struct AST_Binary_Expression : AST_Expression {
    AST_Binary_Expression(): AST_Expression(EXPR_BINARY) {}

    Token_Type op = TOKEN_NONE;
    AST_Expression *left = NULL;
    AST_Expression *right = NULL;
};


struct AST_Function_Call : AST_Expression {
    AST_Function_Call(): AST_Expression(EXPR_FUNC_CALL) {}

    std::string function_name;
    std::vector<AST_Expression*> params;
};


struct AST_Return_Expression : AST_Expression {
    AST_Return_Expression(): AST_Expression(EXPR_RETURN) {}

    AST_Expression *value = NULL;
};


struct AST_Jump_Expression : AST_Expression {
    AST_Jump_Expression(): AST_Expression(EXPR_JUMP) {}

    std::string jump_type;
};


// for scoped expression blocks
// (other than the ones attached to if/for/while/functions...)
struct AST_Block_Expression : AST_Expression {
    AST_Block_Expression(): AST_Expression(EXPR_BLOCK) {}

    std::vector<AST_Expression*> block;
};


//                codegen helper functions
// *****************************************************

llvm::Value* codegen__logical_and(AST_Expression* left, AST_Expression* right) {
    llvm::Function* f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* andright = llvm::BasicBlock::Create(_context, "andright", f);
    llvm::BasicBlock* andend = llvm::BasicBlock::Create(_context, "andend", f);

    // evaluate left
    llvm::Value* L = left->codegen();
    if (!L) return nullptr;

    _builder.CreateCondBr(L, andright, andend);

    // right block
    _builder.SetInsertPoint(andright);
    llvm::Value* R = right->codegen();
    if (!R) return nullptr;
    _builder.CreateBr(andend);

    // end block
    _builder.SetInsertPoint(andend);
    llvm::PHINode* PN = _builder.CreatePHI(llvm::Type::getInt1Ty(_context), 2, "andtmp");
    PN->addIncoming(llvm::ConstantInt::getFalse(_context), _builder.GetInsertBlock()->getPrevNode());
    PN->addIncoming(R, andright);

    return PN;
}


llvm::Value* codegen__logical_or(AST_Expression* left, AST_Expression* right) {
    llvm::Function* f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* orright = llvm::BasicBlock::Create(_context, "orright", f);
    llvm::BasicBlock* orend = llvm::BasicBlock::Create(_context, "orend", f);

    // evaluate left
    llvm::Value* L = left->codegen();
    if (!L) return nullptr;

    _builder.CreateCondBr(L, orend, orright);

    // right block
    _builder.SetInsertPoint(orright);
    llvm::Value* R = right->codegen();
    if (!R) return nullptr;
    _builder.CreateBr(orend);

    // end block
    _builder.SetInsertPoint(orend);
    llvm::PHINode* PN = _builder.CreatePHI(llvm::Type::getInt1Ty(_context), 2, "ortmp");
    PN->addIncoming(llvm::ConstantInt::getTrue(_context), _builder.GetInsertBlock()->getPrevNode());
    PN->addIncoming(R, orright);

    return PN;
}

#endif
