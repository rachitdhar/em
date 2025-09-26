//
// ir_generator.cpp
//

/*

Here we shall define all the generate_ir functions
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



// for variables, we can return two kinds of quantities
// either we could directly return its value, or we could
// return a pointer to the variable.
//
//   generate_ir() -> returns the value of the expression
//   generate_ir_pointer() -> returns the address of the "expression" (variable)
//
// this only applies for lvalue expressions (identifiers)
inline llvm::Value *AST_Identifier::generate_ir_pointer()
{
    llvm::Value *val_addr = llvm_symbol_table[variable_name];
    if (!val_addr) {
	throw_ir_error("Undefined identifier encountered.");
    }
    return val_addr;
}


inline llvm::Value *AST_Identifier::generate_ir()
{
    // returns the value contained in a particular variable
    llvm::Value *val_addr = generate_ir_pointer();
    return _builder.CreateLoad(val_addr->getType()->getPointerElementType(), val_addr, name.c_str());
}


inline llvm::Value *AST_Literal::generate_ir()
{
    switch (type) {
    case T_INT:    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(_builder.getContext()), value.i);
    case T_FLOAT:  return llvm::ConstantFP::get(llvm::Type::getFloatTy(_builder.getContext()), value.f);
    case T_CHAR:   return llvm::ConstantInt::get(llvm::Type::getInt8Ty(_builder.getContext()), value.c);
    case T_STRING: return _builder.CreateGlobalStringPtr(*(value.s));
    default: throw_ir_error("Unidentified literal type encountered.");
    }
}


inline llvm::Value *AST_Function_Definition::generate_ir()
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

	// store in the symbol table
	llvm_symbol_table[std::string(arg.getName())] = _alloca;
    }

    // entry block for function body
    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(_context, "entry", _f);
    _builder.SetInsertPoint(function_entry);

    // emit body
    for (auto* expr : block) {
        if (!expr->generate_ir()) return nullptr;
    }

    // if return type is void, add a return void instruction
    // (for other types, the return expression should be present
    // in the block itself).
    if (llvm_return_type->isVoidTy()) _builder.CreateRetVoid();

    // verify function
    if (llvm::verifyFunction(*_f, &llvm::errs())) {
	throw_ir_error("Invalid function. Could not be verified.");
    }

    return _f;
}


inline llvm::Value *AST_If_Expression::generate_ir()
{
    // %ifcond = icmp ne i32 %x, 0
    llvm::Value *_condition = condition->generate_ir();
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
        if (!expr->generate_ir()) return nullptr;
    }
    _builder.CreateBr(_ifend);
    _then = _builder.GetInsertBlock();

    // emit instructions for _else block
    _f->getBasicBlockList().push_back(_else);
    _builder.SetInsertPoint(_else);
    for (auto *expr : else_block) {
        if (!expr->generate_ir()) return nullptr;
    }
    _builder.CreateBr(_ifend);
    _else = _builder.GetInsertBlock();

    // emit _ifend label (marks the termination of the if statement)
    _f->getBasicBlockList().push_back(_ifend);
    _builder.SetInsertPoint(_ifend);

    return nullptr;  // if statement returns no value
}


inline llvm::Value *AST_For_Expression::generate_ir()
{
    llvm::Function *f = _builder.GetInsertBlock()->getParent();

    // emit init
    if (init) init->generate_ir();

    llvm::BasicBlock *_forcond = llvm::BasicBlock::Create(_context, "forcond", f);
    llvm::BasicBlock *_forbody = llvm::BasicBlock::Create(_context, "forbody");
    llvm::BasicBlock *_forinc  = llvm::BasicBlock::Create(_context, "forinc");
    llvm::BasicBlock *_forend = llvm::BasicBlock::Create(_context, "forend");

    // jump to condition check
    _builder.CreateBr(_forcond);

    _builder.SetInsertPoint(_forcond);
    llvm::Value *_condition = condition ? condition->generate_ir() : nullptr;

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

    auto *terminals = new Loop_Terminals;
    terminals->loop_condition = _condition;
    terminals->loop_end = _forend;
    loop_terminals.push(terminals);

    // emit body
    f->getBasicBlockList().push_back(_forbody);
    _builder.SetInsertPoint(_forbody);

    for (auto *expr : block) {
        if (!expr->generate_ir()) return nullptr;
    }

    // after the body, jump to increment
    _builder.CreateBr(_forinc);

    // increment
    f->getBasicBlockList().push_back(_forinc);
    _builder.SetInsertPoint(_forinc);

    if (increment) increment->generate_ir();

    // jump back to condition
    _builder.CreateBr(_forcond);

    // for end
    f->getBasicBlockList().push_back(_forend);
    _builder.SetInsertPoint(_forend);

    loop_terminals.pop();
    return nullptr; // for statement doesn't return any value
}


inline llvm::Value *AST_While_Expression::generate_ir()
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
    llvm::Value* _condition = condition->generate_ir();
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
        if (!expr->generate_ir()) return nullptr;
    }

    // jump back to condition
    _builder.CreateBr(_condition);
    _body = _builder.GetInsertBlock();

    // _whileend (for termination)
    f->getBasicBlockList().push_back(_whileend);
    _builder.SetInsertPoint(_whileend);

    loop_terminals.pop();
    return nullptr; // while statement returns no value
}


inline llvm::Value *AST_Declaration::generate_ir()
{
    llvm::Function *f = _builder.GetInsertBlock()->getParent();

    // create alloca at the beginning of the entry block
    llvm::IRBuilder<> tmp_builder(&_context);
    llvm::BasicBlock &EntryBlock = f->getEntryBlock();
    tmp_builder.SetInsertPoint(&EntryBlock, EntryBlock.begin());

    llvm::Type *var_type = llvm_type_map(data_type);
    llvm::AllocaInst *_alloca = tmp_builder.CreateAlloca(var_type, nullptr, variable_name);

    // store it in the symbol table
    llvm_symbol_table[variable_name] = _alloca;
    return _alloca;
}


inline llvm::Value *AST_Unary_Expression::generate_ir()
{
    llvm::Value* val = expr->generate_ir();

    switch (op) {
    case TOKEN_NOT: {
	// get a 0 having a type same as val
        llvm::Value *zero = llvm::ConstantInt::get(val->getType(), 0);
        return _builder.CreateICmpEQ(val, zero, "nottmp");
    }
    case TOKEN_BIT_NOT: {
	// get an integer with all bits set to 1, of the same type as val
        llvm::Value *all_ones = llvm::ConstantInt::get(val->getType(), -1, true);
        return _builder.CreateXor(val, all_ones, "bnot");
    }
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT: {
	// for increments / decrements, the expression
	// must be an lvalue, and we will need the address to it,
	// instead of its direct value.

	if (expr_type != EXPR_IDENT) {
	    throw_ir_error("Cannot increment/decrement a non-lvalue expression).");
	}

        llvm::Value *val_addr = ((AST_Identifier*)expr)->generate_ir_pointer();
        llvm::Value *old_val =
	_builder.CreateLoad(val_addr->getType()->getPointerElementType(), val_addr, "oldtmp");

        int delta = (op == TOKEN_INCREMENT) ? 1 : -1;
        llvm::Value *one = llvm::ConstantInt::get(old_val->getType(), delta);
        llvm::Value *new_val = _builder.CreateAdd(old_val, one, "incdec");

        _builder.CreateStore(new_val, val_addr);
	return (is_postfix) ? old_val : new_val;
    }
    default: throw_ir_error("Invalid unary operator encountered.");
    }
}


inline llvm::Value *AST_Binary_Expression::generate_ir()
{
    // a binary operation could either be a kind
    // of assignment, or a logical operation, or
    // some binary operation. for each case we will have
    // to handle the code generation accordingly.

    /*
    For Logical AND / Logical OR:

	These must be handled separately, as we will implement
	short-circuiting behavior for them (i.e., we only evaluate
	the right expression if needed)
    */

    if (op == TOKEN_AND) return generate_ir__logical_and(left, right);
    if (op == TOKEN_OR) return generate_ir__logical_or(left, right);

    llvm::Value* L = left->generate_ir();
    if (!L) return nullptr;

    llvm::Value* R = right->generate_ir();
    if (!R) return nullptr;

    // For other logical / bitwise operations
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
        default: break;
    }

    /*
    For Assignments:

	Either we could have a normal assignment (=)
	Or, a combination of some binary operation and assignment (like +=, -=, etc.)
    */

    if (op == TOKEN_ASSIGN) {
	_builder.CreateStore(R, L);
        return R;
    }

    llvm::Value *tmp = _builder.CreateLoad(L->getType()->getPointerElementType(), L);
    llvm::Value *res;

    switch (op) {
    case TOKEN_PLUSEQ: res = _builder.CreateAdd(tmp, R, "addtmp"); break;
    case TOKEN_MINUSEQ: res = _builder.CreateSub(tmp, R, "subtmp"); break;
    case TOKEN_MULTIPLYEQ: res = _builder.CreateMul(tmp, R, "multmp"); break;
    case TOKEN_DIVIDEEQ: res = _builder.CreateSDiv(tmp, R, "divtmp"); break;
    case TOKEN_MODEQ: res = _builder.CreateSRem(tmp, R, "modtmp"); break;
    case TOKEN_LSHIFT_EQ: res = _builder.CreateShl(tmp, R, "lshtmp"); break;
    case TOKEN_RSHIFT_EQ: res = _builder.CreateAShr(tmp, R, "rshtmp"); break;
    case TOKEN_ANDEQ: res = _builder.CreateAnd(tmp, R, "andtmp"); break;
    case TOKEN_OREQ: res = _builder.CreateOr(tmp, R, "ortmp"); break;
    case TOKEN_BIT_ANDEQ: res = _builder.CreateAnd(tmp, R, "andtmp"); break;
    case TOKEN_BIT_OREQ: res = _builder.CreateOr(tmp, R, "ortmp"); break;
    case TOKEN_XOREQ: res = _builder.CreateXor(tmp, R, "xortmp"); break;
    default: break;
    }

    _builder.CreateStore(res, L);
    return res;
}


inline llvm::Value *AST_Function_Call::generate_ir()
{
    // find the function in the module
    llvm::Function* callee = _module->getFunction(function_name);
    if (!callee) {
	throw_ir_error("Invalid function call.");
    }

    // generate instructions for each argument
    std::vector<llvm::Value*> args;
    for (auto* param : params) {
        llvm::Value* arg_val = param->generate_ir();
        if (!arg_val) return nullptr;
        args.push_back(argVal);
    }

    // emit the call instruction
    return _builder.CreateCall(callee, args, "calltmp");
}


inline llvm::Value *AST_Return_Expression::generate_ir()
{
    if (value) {
        llvm::Value *val = value->generate_ir();
        if (!val) return nullptr;
        return _builder.CreateRet(val); // ret <value>
    }
    return _builder.CreateRetVoid(); // ret void
}


inline llvm::Value *AST_Jump_Expression::generate_ir()
{
    // we just peek at the top of the loop stack
    // to get to know the label of the condition/end
    // of the loop where we need to jump to.

    if (loop_terminals.empty()) {
	throw_ir_error("\'break\'/\'continue\' cannot be used outside a loop.");
    }

    Loop_Terminals *current = loop_terminals.top();

    switch (jump_type) {
    case J_BREAK: _builder.CreateBr(current->loop_end); break;
    case J_CONTINUE: _builder.CreateBr(current->loop_condition); break;
    default: throw_ir_error("Invalid jump type encountered.");
    }

    llvm::Function* f = _builder.GetInsertBlock()->getParent();
    auto *jumpend = llvm::BasicBlock::Create(_context, "jumpend", f);
    _builder.SetInsertPoint(jumpend);

    return nullptr; // break and continue don't return any value
}


inline llvm::Value *AST_Block_Expression::generate_ir()
{
    llvm::Value *prev = nullptr;

    for (auto* expr : block) {
        prev = expr->generate_ir();
        if (!prev) return nullptr;
    }
    return nullptr; // scoped-expressions don't return any value
}
