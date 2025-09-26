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
    // get the llvm return type
    llvm::Type *llvm_return_type = llvm_type_map(return_type);

    // get the llvm parameter types
    std::vector<llvm::Type*> llvm_param_types;
    for (auto* param : params) {
	llvm_param_types.push_back(llvm_type_map(param->type));
    }

    llvm::FunctionType* f_type = llvm::FunctionType::get(llvm_return_type, llvm_param_types, false);
    llvm::Function* _f = llvm::Function::Create(
        f_type,
        llvm::Function::ExternalLinkage,
        function_name,
        _module
    );

    // set parameter names and allocate storage
    int index = 0;
    for (auto& arg : _f->args()) {
        arg.setName(params[index++]->name);

        // we need to create an alloca in the entry block, for this parameter.
	// in order to do this, we can create a temporary builder, set its
	// insertion point to the entry, and insert the instructions there,
	// without affecting the global builder.
        llvm::IRBuilder<> tmp_builder(&_context);

        tmp_builder.SetInsertPoint(
	&_builder.GetInsertBlock()->getParent()->getEntryBlock(),
        _builder.GetInsertBlock()->getParent()->getEntryBlock().begin()
	);
        llvm::AllocaInst* _alloca = tmp_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());

        // store the initial parameter value
        _builder.CreateStore(&arg, _alloca);
    }

    // entry block for function body
    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(_context, "entry", _f);
    _builder.SetInsertPoint(function_entry);

    // emit body
    for (auto* expr : block) {
        if (!expr->codegen()) return nullptr;
    }

    // if return type is void, add a return void instruction
    // (for other types, the return expression should be present
    // in the block itself).
    if (llvm_return_type->isVoidTy()) _builder.CreateRetVoid();

    // verify function
    if (llvm::verifyFunction(*_f, &llvm::errs())) {
        // throw error (function verification failed)
    }

    return _f;
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
    llvm::Function *f = _builder.GetInsertBlock()->getParent();

    // emit init
    if (init) init->codegen();

    llvm::BasicBlock *_forcond = llvm::BasicBlock::Create(_context, "forcond", f);
    llvm::BasicBlock *_forbody = llvm::BasicBlock::Create(_context, "forbody");
    llvm::BasicBlock *_forinc  = llvm::BasicBlock::Create(_context, "forinc");
    llvm::BasicBlock *_forend = llvm::BasicBlock::Create(_context, "forend");

    // jump to condition check
    _builder.CreateBr(_forcond);

    _builder.SetInsertPoint(_forcond);
    llvm::Value *_condition = condition ? condition->codegen() : nullptr;

    if (_condition) {
        _condition = _builder.CreateICmpNE(
            _condition,
            llvm::ConstantInt::get(_condition->getType(), 0),
            "forcond"
        );
    } else {
        // if no condition, then always true
        _condition = llvm::ConstantInt::getTrue(_context);
    }
    _builder.CreateCondBr(_condition, _forbody, _forend);

    // emit body
    f->getBasicBlockList().push_back(_forbody);
    _builder.SetInsertPoint(_forbody);

    for (auto *expr : block) {
        if (!expr->codegen()) return nullptr;
    }

    // after the body, jump to increment
    _builder.CreateBr(_forinc);

    // increment
    f->getBasicBlockList().push_back(_forinc);
    _builder.SetInsertPoint(_forinc);

    if (increment) increment->codegen();

    // jump back to condition
    _builder.CreateBr(_forcond);

    // for end
    f->getBasicBlockList().push_back(_forend);
    _builder.SetInsertPoint(_forend);

    return nullptr;
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
    llvm::Function *f = _builder.GetInsertBlock()->getParent();

    // create alloca at the beginning of the entry block
    llvm::IRBuilder<> tmp_builder(&_context);
    llvm::BasicBlock &EntryBlock = f->getEntryBlock();
    tmp_builder.SetInsertPoint(&EntryBlock, EntryBlock.begin());

    llvm::Type *var_type = llvm_type_map(data_type);
    llvm::AllocaInst *_alloca = tmp_builder.CreateAlloca(var_type, nullptr, variable_name);
    return _alloca;
}


inline llvm::Value *AST_Unary_Expression::codegen()
{

}


inline llvm::Value *AST_Binary_Expression::codegen()
{
    // a binary operation could either be a kind
    // of assignment, or a logical operation, or
    // some binary operation. for each case we will have
    // to handle the code generation accordingly.

    //                For assignments
    // ********************************************
    if (
    op == TOKEN_ASSIGN ||
    op == TOKEN_PLUSEQ ||
    op == TOKEN_MINUSEQ ||
    op == TOKEN_MULTIPLYEQ ||
    op == TOKEN_DIVIDEEQ ||
    op == TOKEN_MODEQ
    ) {
        llvm::Value* L = ((AST_Identifier*)left)->codegen();
        if (!L) return nullptr;

        llvm::Value* R = right->codegen();
        if (!R) return nullptr;

        llvm::Value* res = nullptr;

        switch (op) {
            case TOKEN_ASSIGN:
                _builder.CreateStore(R, L);
                res = R;
                break;
            case TOKEN_PLUSEQ: {
                llvm::Value* Old = _builder.CreateLoad(L->getType()->getPointerElementType(), L);
                res = _builder.CreateAdd(Old, R, "addtmp");
                _builder.CreateStore(Result, L);
                break;
            }
            case TOKEN_MINUSEQ: {
                llvm::Value* Old = _builder.CreateLoad(L->getType()->getPointerElementType(), L);
                res = _builder.CreateSub(Old, R, "subtmp");
                _builder.CreateStore(Result, L);
                break;
            }
        }
        return res;
    }

    /*
    Logical AND / Logical OR:

	These must be handled separately, as we will implement
	short-circuiting behavior for them (i.e., we only evaluate
	the right expression if needed)
    */

    if (op == TOKEN_AND) return codegen__logical_and(left, right);
    if (op == TOKEN_OR) return codegen__logical_or(left, right);

    //            For logical / bitwise operations
    // *****************************************************

    llvm::Value* L = left->codegen();
    llvm::Value* R = right->codegen();
    if (!L || !R) return nullptr;

    switch (op) {
        case TOKEN_PLUS:      return _builder.CreateAdd(L, R, "addtmp");
        case TOKEN_MINUS:     return _builder.CreateSub(L, R, "subtmp");
        case TOKEN_STAR:      return _builder.CreateMul(L, R, "multmp");
        case TOKEN_DIVIDE:    return _builder.CreateSDiv(L, R, "divtmp");
        case TOKEN_MOD:       return _builder.CreateSRem(L, R, "modtmp");
        case TOKEN_LESS:      return _builder.CreateICmpSLT(L, R, "cmptmp");
        case TOKEN_GREATER:   return _builder.CreateICmpSGT(L, R, "cmptmp");
        case TOKEN_LESSEQ:    return _builder.CreateICmpSLE(L, R, "cmptmp");
        case TOKEN_GREATEREQ: return _builder.CreateICmpSGE(L, R, "cmptmp");
        case TOKEN_EQUAL:     return _builder.CreateICmpEQ(L, R, "cmptmp");
        case TOKEN_NOTEQ:     return _builder.CreateICmpNE(L, R, "cmptmp");
        case TOKEN_LSHIFT:    return _builder.CreateShl(L, R, "lshtmp");
        case TOKEN_RSHIFT:    return _builder.CreateAShr(L, R, "rshtmp");
        case TOKEN_BIT_OR:    return _builder.CreateOr(L, R, "ortmp");
        case TOKEN_XOR:       return _builder.CreateXor(L, R, "xortmp");
        case TOKEN_AMPERSAND: return _builder.CreateAnd(L, R, "andtmp");
        default:
            return nullptr;
    }
}


inline llvm::Value *AST_Function_Call::codegen()
{
    // find the function in the module
    llvm::Function* callee = _module->getFunction(function_name);
    if (!callee) {
        // throw error (invalid function call)
    }

    // generate instructions for each argument
    std::vector<llvm::Value*> args;
    for (auto* param : params) {
        llvm::Value* arg_val = param->codegen();
        if (!arg_val) return nullptr;
        args.push_back(argVal);
    }

    // emit the call instruction
    return _builder.CreateCall(callee, args, "calltmp");
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
    // we just peek at the top of the loop stack
    // to get to know the label of the condition/end
    // of the loop where we need to jump to.

    if (loop_terminals.empty()) {
        // throw error (break/continue cannot be used outside a loop)
    }

    Loop_Terminals *current = loop_terminals.top();

    if (jump_type == "break") {
        _builder.CreateBr(current->loop_end);
    } else if (jump_type == "continue") {
        _builder.CreateBr(current->loop_condition);
    } else {
        // throw error (invalid jump type)
    }

    llvm::Function* f = _builder.GetInsertBlock()->getParent();
    auto *jumpend = llvm::BasicBlock::Create(_context, "jumpend", f);
    _builder.SetInsertPoint(jumpend);

    return nullptr;
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
