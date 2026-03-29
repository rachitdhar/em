//
// ast.h
//

#pragma once

#include "tokens.h"
#include "llvm.h"
#include "types.h"
#include "dsa.h"
#include <stack>


enum Expression_Type {
    EXPR_IDENT,
    EXPR_LITERAL,
    EXPR_FUNC_DEF,
    EXPR_IF,
    EXPR_CASE,
    EXPR_SWITCH,
    EXPR_FOR,
    EXPR_WHILE,
    EXPR_DECL,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_FUNC_CALL,
    EXPR_RETURN,
    EXPR_JUMP,
    EXPR_BLOCK,
    EXPR_VARG
};

enum Jump_Type { J_BREAK, J_CONTINUE };

struct Function_Parameter {
    std::string name;
    Data_Type *type = NULL;
};

//                             LLVM Objects
// ********************************************************************

struct LLVM_Symbol_Info {
    llvm::Value *val;
    llvm::Type *type;
};

struct Loop_Terminals {
    llvm::BasicBlock *loop_condition;
    llvm::BasicBlock *loop_end;
};

struct LLVM_IR {
    llvm::LLVMContext &_context;
    llvm::IRBuilder<> *_builder;
    llvm::Module *_module;

    // declaring a symbol table needed during IR generation
    // apparently, we would need to store both the value as well
    // as the type (since LLVM only keeps a pointer to the allocated
    // memory, and does not seem to maintain type info).
    smap<LLVM_Symbol_Info *> llvm_symbol_table;

    // defining a stack to store the pairs <LOOP_CONDITION, LOOP_END>
    // whenever we encounter a while/for loop. this is needed
    // to be able to jump to those locations when a break/continue
    // statement is encountered.
    std::stack<Loop_Terminals *> loop_terminals;

    // we need this to keep track of the variadic args list
    // (va_list), throughout a function body, which goes across
    // different expressions, and where we might need this to call
    // va_arg over there.
    llvm::Value *current_va_list = nullptr;

    // to keep track of the current switchend position
    // so that it can be used inside a case block
    llvm::BasicBlock *current_switch_end = nullptr;

    LLVM_IR(llvm::LLVMContext &c, llvm::IRBuilder<> *b, llvm::Module *m)
        : _context(c), _builder(b), _module(m) {}
};

//                           AST Expressions
// ********************************************************************

struct AST_Expression {
    Expression_Type expr_type = EXPR_IDENT;

    // set the expression type for a derived struct
    AST_Expression(Expression_Type type) : expr_type(type) {}
    AST_Expression() : expr_type(EXPR_IDENT) {}

    // to ensure that in case a derived struct is deleted,
    // the derived struct's destructor is called
    virtual ~AST_Expression() = default;

    virtual llvm::Value *generate_ir(LLVM_IR *ir) = 0;
};

struct AST_Identifier : AST_Expression {
    AST_Identifier() : AST_Expression(EXPR_IDENT) {}

    std::string name;

    llvm::Value *generate_ir_pointer(LLVM_IR *ir); // for handling lvalues
    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Literal : AST_Expression {
    AST_Literal() : AST_Expression(EXPR_LITERAL) {}

    Data_Type *type = NULL;

    union {
        bool b;
        int8_t i_s8;
        int16_t i_s16;
        int32_t i_s32;
        int64_t i_s64;
        uint8_t i_u8;
        uint16_t i_u16;
        uint32_t i_u32;
        uint64_t i_u64;
        float f_32;
        double f_64;
        std::string *s; // storing the address of the string
    } value;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Function_Definition : AST_Expression {
    AST_Function_Definition() : AST_Expression(EXPR_FUNC_DEF) {}

    bool is_prototype = false;
    bool has_variadic_args = false;

    Data_Type *return_type = NULL;
    std::string function_name;

    std::vector<Function_Parameter *> params;
    std::vector<AST_Expression *> block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_If_Expression : AST_Expression {
    AST_If_Expression() : AST_Expression(EXPR_IF) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression *> block;
    std::vector<AST_Expression *> else_block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Case_Expression : AST_Expression {
    AST_Case_Expression() : AST_Expression(EXPR_CASE) {}

    AST_Literal *literal = NULL; // this is NULL for the "default" case
    std::vector<AST_Expression *> block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Switch_Expression : AST_Expression {
    AST_Switch_Expression() : AST_Expression(EXPR_SWITCH) {}

    bool has_default_case = false;
    AST_Expression *identifier_or_call = NULL;
    std::vector<AST_Case_Expression *> case_list;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_For_Expression : AST_Expression {
    AST_For_Expression() : AST_Expression(EXPR_FOR) {}

    AST_Expression *init = NULL;
    AST_Expression *condition = NULL;
    AST_Expression *increment = NULL;
    std::vector<AST_Expression *> block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_While_Expression : AST_Expression {
    AST_While_Expression() : AST_Expression(EXPR_WHILE) {}

    AST_Expression *condition = NULL;
    std::vector<AST_Expression *> block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Declaration : AST_Expression {
    AST_Declaration() : AST_Expression(EXPR_DECL) {}

    Data_Type *data_type = NULL;
    std::string variable_name;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Unary_Expression : AST_Expression {
    AST_Unary_Expression() : AST_Expression(EXPR_UNARY) {}

    bool is_postfix = false;
    Token_Type op = TOKEN_NONE;
    AST_Expression *expr = NULL;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Binary_Expression : AST_Expression {
    AST_Binary_Expression() : AST_Expression(EXPR_BINARY) {}

    Token_Type op = TOKEN_NONE;
    AST_Expression *left = NULL;
    AST_Expression *right = NULL;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Function_Call : AST_Expression {
    AST_Function_Call() : AST_Expression(EXPR_FUNC_CALL) {}

    std::string function_name;
    std::vector<AST_Expression *> params;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Return_Expression : AST_Expression {
    AST_Return_Expression() : AST_Expression(EXPR_RETURN) {}

    AST_Expression *value = NULL;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Jump_Expression : AST_Expression {
    AST_Jump_Expression() : AST_Expression(EXPR_JUMP) {}

    Jump_Type jump_type;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

struct AST_Varg : AST_Expression {
    AST_Varg() : AST_Expression(EXPR_VARG) {}

    Data_Type *data_type = NULL;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};

// for scoped expression blocks
// (other than the ones attached to if/for/while/functions...)
struct AST_Block_Expression : AST_Expression {
    AST_Block_Expression() : AST_Expression(EXPR_BLOCK) {}

    std::vector<AST_Expression *> block;

    llvm::Value *generate_ir(LLVM_IR *ir) override;
};
