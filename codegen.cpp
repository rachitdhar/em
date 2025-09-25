//
// codegen.cpp
//

/*

Here we shall define all the codegen functions
for the AST expressions. This involves specifying
what instructions are supposed to be generated for
every kind of expression that we encounter.

This is one of the most crucial components practically,
after the AST itself, because everything our compiler does,
if it is to be successful, ultimately boils down to whether
it is actually generating something executable on the machine.
To get there we need an intermediate LLVM "assembly" (which
is hardware architecture independent).

*/


#include "ast.h"
#include <stack>


// defining a stack to store the pairs <LOOP_CONDITION, LOOP_END>
// whenever we encounter a while/for loop. this is needed
// to be able to jump to those locations when a break/continue
// statement is encountered.

struct Loop_Terminals {
    llvm::BasicBlock *loop_condition;
    llvm::BasicBlock *loop_end;
};

std::stack<Loop_Terminals*> loop_terminals;



inline llvm::Value *AST_Identifier::codegen()
{
    // this is an expression that just returns
    // the value contained in a particular variable
}


inline llvm::Value *AST_Literal::codegen()
{
    // returns the literal
}


inline llvm::Value *AST_Function_Definition::codegen()
{

}


inline llvm::Value *AST_If_Expression::codegen()
{
    // %ifcond = icmp ne i32 %x, 0
    llvm::Value *_condition = condition->codegen();
    if (!_condition) return nullptr;

    _condition = _builder.CreateICmpNE(
    _condition,
    llvm::ConstantInt::get(_condition->getType(), 0),
    "ifcond"
    );

    // create labels (then:, else:, and ifend:)
    llvm::Function *_f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *_then = llvm::BasicBlock::Create(_context, "then", f);
    llvm::BasicBlock *_else  = llvm::BasicBlock::Create(_context, "else");
    llvm::BasicBlock *_ifend = llvm::BasicBlock::Create(_context, "ifend");

    // create conditional branch
    _builder.CreateCondBr(_condition, _then, _else);

    // emit instructions for _then block
    _builder.SetInsertPoint(_then);
    for (auto *expr : block) {
        if (!expr->codegen()) return nullptr;
    }
    _builder.CreateBr(_ifend);
    _then = _builder.GetInsertBlock();

    // emit instructions for _else block
    _f->getBasicBlockList().push_back(_else);
    _builder.SetInsertPoint(_else);
    for (auto *expr : else_block) {
        if (!expr->codegen()) return nullptr;
    }
    _builder.CreateBr(_ifend);
    _else = _builder.GetInsertBlock();

    // emit _ifend label (marks the termination of the if statement)
    _f->getBasicBlockList().push_back(_ifend);
    _builder.SetInsertPoint(_ifend);

    return nullptr;  // if statement returns no value
}


inline llvm::Value *AST_For_Expression::codegen()
{

}


inline llvm::Value *AST_While_Expression::codegen()
{
    // here we will need labels for the
    // while condition, while body,
    // and the while end

    llvm::Function* f = _builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* _condition  = llvm::BasicBlock::Create(_context, "whilecond", f);
    llvm::BasicBlock* _body  = llvm::BasicBlock::Create(_context, "whilebody");
    llvm::BasicBlock* _whileend = llvm::BasicBlock::Create(_context, "whileend");

    // jump to condition block first
    _builder.CreateBr(_condition);

    // emit condition
    _builder.SetInsertPoint(_condition);
    llvm::Value* _condition = condition->codegen();
    if (!_condition) return nullptr;

    _condition = _builder.CreateICmpNE(
        _condition,
        llvm::ConstantInt::get(_condition->getType(), 0),
        "whilecond"
    );

    auto *terminals = new Loop_Terminals;
    terminals->loop_condition = _condition;
    terminals->loop_end = _whileend;
    loop_terminals.push(terminals);

    // create conditional branch
    _builder.CreateCondBr(_condition, _body, _whileend);

    // emit body
    f->getBasicBlockList().push_back(_body);
    _builder.SetInsertPoint(_body);

    for (auto* expr : block) {
        if (!expr->codegen()) return nullptr;
    }

    // jump back to condition
    _builder.CreateBr(_condition);
    _body = _builder.GetInsertBlock();

    // _whileend (for termination)
    f->getBasicBlockList().push_back(_whileend);
    _builder.SetInsertPoint(_whileend);

    return nullptr; // while statement returns no value
}


inline llvm::Value *AST_Declaration::codegen()
{

}


inline llvm::Value *AST_Unary_Expression::codegen()
{

}


inline llvm::Value *AST_Binary_Expression::codegen()
{

}


inline llvm::Value *AST_Function_Call::codegen()
{

}


inline llvm::Value *AST_Return_Expression::codegen()
{
    if (value) {
        llvm::Value *val = value->codegen();
        if (!val) return nullptr;
        return _builder.CreateRet(val); // ret <value>
    }
    return _builder.CreateRetVoid(); // ret void
}


inline llvm::Value *AST_Jump_Expression::codegen()
{

}


inline llvm::Value *AST_Block_Expression::codegen()
{
    llvm::Value *prev = nullptr;

    for (auto* expr : block) {
        prev = expr->codegen();
        if (!prev) return nullptr;
    }
    return nullptr;
}
