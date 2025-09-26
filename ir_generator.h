//
// ir_generator.h
//

/*

This is the file where we define the things we need to generate the
LLVM intermediate representation (IR).

First we have to define the context, module and builder. These are
used in the codegen() function in each AST expression struct,
to help describe what IR instructions are meant to be generated.

*/

#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <unordered_map>
#include <stack>

#include "parser.h"



// declaring a symbol table needed during IR generation
std::unordered_map<std::string, llvm::Value*> llvm_symbol_table;


// defining a stack to store the pairs <LOOP_CONDITION, LOOP_END>
// whenever we encounter a while/for loop. this is needed
// to be able to jump to those locations when a break/continue
// statement is encountered.

struct Loop_Terminals {
    llvm::BasicBlock *loop_condition;
    llvm::BasicBlock *loop_end;
};

std::stack<Loop_Terminals*> loop_terminals;


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


llvm::Value* generate_ir__logical_and(AST_Expression* left, AST_Expression* right) {
    llvm::Function* f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* andright = llvm::BasicBlock::Create(_context, "andright", f);
    llvm::BasicBlock* andend = llvm::BasicBlock::Create(_context, "andend", f);

    // evaluate left
    llvm::Value* L = left->generate_ir();
    if (!L) return nullptr;

    _builder.CreateCondBr(L, andright, andend);

    // right block
    _builder.SetInsertPoint(andright);
    llvm::Value* R = right->generate_ir();
    if (!R) return nullptr;
    _builder.CreateBr(andend);

    // end block
    _builder.SetInsertPoint(andend);
    llvm::PHINode* phi_node = _builder.CreatePHI(llvm::Type::getInt1Ty(_context), 2, "andtmp");
    phi_node->addIncoming(llvm::ConstantInt::getFalse(_context), _builder.GetInsertBlock()->getPrevNode());
    phi_node->addIncoming(R, andright);

    return phi_node;
}


llvm::Value* generate_ir__logical_or(AST_Expression* left, AST_Expression* right) {
    llvm::Function* f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* orright = llvm::BasicBlock::Create(_context, "orright", f);
    llvm::BasicBlock* orend = llvm::BasicBlock::Create(_context, "orend", f);

    // evaluate left
    llvm::Value* L = left->generate_ir();
    if (!L) return nullptr;

    _builder.CreateCondBr(L, orend, orright);

    // right block
    _builder.SetInsertPoint(orright);
    llvm::Value* R = right->generate_ir();
    if (!R) return nullptr;
    _builder.CreateBr(orend);

    // end block
    _builder.SetInsertPoint(orend);
    llvm::PHINode* phi_node = _builder.CreatePHI(llvm::Type::getInt1Ty(_context), 2, "ortmp");
    phi_node->addIncoming(llvm::ConstantInt::getTrue(_context), _builder.GetInsertBlock()->getPrevNode());
    phi_node->addIncoming(R, orright);

    return phi_node;
}


// to print an error message, for errors that occur
// during the LLVM IR generation process.
void throw_ir_error(const char *message)
{
    fprintf(stderr, "IR ERROR: %s", message);
    exit(1);
}

#endif
