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

#include "ir_generator.h"



// for variables, we can return two kinds of quantities
// either we could directly return its value, or we could
// return a pointer to the variable.
//
//   generate_ir() -> returns the value of the expression
//   generate_ir_pointer() -> returns the address of the "expression" (variable)
//
// this only applies for lvalue expressions (identifiers)
inline llvm::Value *AST_Identifier::generate_ir_pointer(LLVM_IR *ir)
{
    LLVM_Symbol_Info *sym_info = ir->llvm_symbol_table[name];
    if (sym_info == NULL || !sym_info->val) {
	throw_ir_error("Undefined identifier encountered.");
    }
    return sym_info->val; // a pointer to the identifier
}


inline llvm::Value *AST_Identifier::generate_ir(LLVM_IR *ir)
{
    // returns the value contained in a particular variable
    LLVM_Symbol_Info *sym_info = ir->llvm_symbol_table[name];
    if (sym_info == NULL) {
	throw_ir_error("Undefined identifier encountered.");
    }

    return ir->_builder->CreateLoad(sym_info->type, sym_info->val, name.c_str());
}


inline llvm::Value *AST_Literal::generate_ir(LLVM_IR *ir)
{
    switch (type) {
    case T_BOOL:   return llvm::ConstantInt::get(llvm::Type::getInt1Ty(ir->_builder->getContext()), value.b ? 1 : 0);
    case T_INT:    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(ir->_builder->getContext()), value.i);
    case T_FLOAT:  return llvm::ConstantFP::get(llvm::Type::getFloatTy(ir->_builder->getContext()), value.f);
    case T_CHAR:   return llvm::ConstantInt::get(llvm::Type::getInt8Ty(ir->_builder->getContext()), value.c);
    case T_STRING: {
	// in LLVM, if a string literal is global, then it
	// must be created as a constant. we need to thus
	// check for that.

	// for global string literal
	if (!ir->_builder->GetInsertBlock()) {
	    // create an array of i8 with null terminator
	    llvm::Constant *strConstant = llvm::ConstantDataArray::getString(ir->_context, *(value.s), true);

	    // create a global to hold the array
	    auto *global_str = new llvm::GlobalVariable(
	    *(ir->_module),
	    strConstant->getType(),
	    true, // isConstant
	    llvm::GlobalValue::PrivateLinkage,
	    strConstant,
	    ".str"
	    );

	    global_str->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
	    global_str->setAlignment(llvm::Align(1));
	    return global_str;
	}
	return ir->_builder->CreateGlobalStringPtr(*(value.s)); // local string literal
    }
    default: throw_ir_error("Unidentified literal type encountered.");
    }
    return nullptr; // added to prevent warnings (should never reach here)
}


inline llvm::Value *AST_Function_Definition::generate_ir(LLVM_IR *ir)
{
    // get the llvm return type
    llvm::Type *llvm_return_type = llvm_type_map(return_type, ir->_context);

    // get the llvm parameter types
    std::vector<llvm::Type*> llvm_param_types;
    for (auto* param : params) {
      llvm_param_types.push_back(llvm_type_map(param->type, ir->_context));
    }

    llvm::FunctionType* f_type = llvm::FunctionType::get(llvm_return_type, llvm_param_types, false);
    llvm::Function* _f = llvm::Function::Create(
        f_type,
        llvm::Function::ExternalLinkage,
        function_name,
        ir->_module
    );

    // if this is just a prototype, then we don't need to
    // create a basic block. just declaring the function is
    // all that's needed.
    if (is_prototype) return _f;

    // create the entry block
    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(ir->_context, "entry", _f);
    ir->_builder->SetInsertPoint(function_entry);

    // set parameter names and allocate storage
    int index = 0;
    for (auto& arg : _f->args()) {
	std::string param_name = params[index++]->name;
        arg.setName(param_name);

        // we need to create an alloca in the entry block, for this parameter.
	// in order to do this, we can create a temporary builder, set its
	// insertion point to the entry, and insert the instructions there,
	// without affecting the global builder.
        llvm::IRBuilder<> tmp_builder(ir->_context);
        tmp_builder.SetInsertPoint(function_entry, function_entry->begin());

	llvm::AllocaInst* _alloca = tmp_builder.CreateAlloca(arg.getType(), nullptr, param_name);

        // store the initial parameter value
        ir->_builder->CreateStore(&arg, _alloca);

	// store in the symbol table
	auto *sym_info = new LLVM_Symbol_Info{ _alloca, arg.getType() };
	ir->llvm_symbol_table.insert(param_name, sym_info);
    }

    // emit body
    bool has_return_expression_in_block = generate_block_ir(ir, block);

    // if return type is void, add a return void instruction
    // (for other types, the return expression should be present
    // in the block itself).
    if (!has_return_expression_in_block && llvm_return_type->isVoidTy()) {
	ir->_builder->CreateRetVoid();
    }

    // verify function
    if (llvm::verifyFunction(*_f, &llvm::errs())) {
	print_ir(ir->_module);
	throw_ir_error("Invalid function. Could not be verified.");
    }

    return _f;
}


inline llvm::Value *AST_If_Expression::generate_ir(LLVM_IR *ir)
{
    // %ifcond = icmp ne i32 %x, 0
    llvm::Value *_condition = condition->generate_ir(ir);
    if (!_condition) return nullptr;

    _condition = ir->_builder->CreateICmpNE(
    _condition,
    llvm::ConstantInt::get(_condition->getType(), 0),
    "ifcond"
    );

    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for \'if\' statement.");
    }

    // create labels (then:, else:, and ifend:)
    llvm::Function *_f = ir->_builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *_then = llvm::BasicBlock::Create(ir->_context, "then", _f);
    llvm::BasicBlock *_else  = llvm::BasicBlock::Create(ir->_context, "else", _f);
    llvm::BasicBlock *_ifend = llvm::BasicBlock::Create(ir->_context, "ifend", _f);

    // create conditional branch
    ir->_builder->CreateCondBr(_condition, _then, _else);

    // emit instructions for _then block
    ir->_builder->SetInsertPoint(_then);
    bool has_return_expression_in_block = generate_block_ir(ir, block);
    if (!has_return_expression_in_block) ir->_builder->CreateBr(_ifend);
    _then = ir->_builder->GetInsertBlock();

    // emit instructions for _else block
    _else->insertInto(_f);
    ir->_builder->SetInsertPoint(_else);
    has_return_expression_in_block = generate_block_ir(ir, block);
    if (!has_return_expression_in_block) ir->_builder->CreateBr(_ifend);
    _else = ir->_builder->GetInsertBlock();

    // emit _ifend label (marks the termination of the if statement)
    _ifend->insertInto(_f);
    ir->_builder->SetInsertPoint(_ifend);

    return nullptr;  // if statement returns no value
}


inline llvm::Value *AST_For_Expression::generate_ir(LLVM_IR *ir)
{
    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for \'for\' statement.");
    }

    llvm::Function *f = ir->_builder->GetInsertBlock()->getParent();

    // emit init
    if (init) init->generate_ir(ir);

    llvm::BasicBlock *_forcond = llvm::BasicBlock::Create(ir->_context, "forcond", f);
    llvm::BasicBlock *_forbody = llvm::BasicBlock::Create(ir->_context, "forbody", f);
    llvm::BasicBlock *_forinc  = llvm::BasicBlock::Create(ir->_context, "forinc", f);
    llvm::BasicBlock *_forend = llvm::BasicBlock::Create(ir->_context, "forend", f);

    // jump to condition check
    ir->_builder->CreateBr(_forcond);

    ir->_builder->SetInsertPoint(_forcond);
    llvm::Value *_condition = condition
    ? condition->generate_ir(ir) : nullptr;

    if (_condition) {
        _condition = ir->_builder->CreateICmpNE(
            _condition,
            llvm::ConstantInt::get(_condition->getType(), 0),
            "forcond"
        );
    } else {
        // if no condition, then always true
        _condition = llvm::ConstantInt::getTrue(ir->_context);
    }
    ir->_builder->CreateCondBr(_condition, _forbody, _forend);

    auto *terminals = new Loop_Terminals;
    terminals->loop_condition = _forcond;
    terminals->loop_end = _forend;
    ir->loop_terminals.push(terminals);

    // emit body
    _forbody->insertInto(f);
    ir->_builder->SetInsertPoint(_forbody);

    bool has_return_expression_in_block = generate_block_ir(ir, block);

    // after the body, jump to increment
    if (!has_return_expression_in_block) ir->_builder->CreateBr(_forinc);

    // increment
    _forinc->insertInto(f);
    ir->_builder->SetInsertPoint(_forinc);

    if (increment) increment->generate_ir(ir);

    // jump back to condition
    ir->_builder->CreateBr(_forcond);

    // for end
    _forend->insertInto(f);
    ir->_builder->SetInsertPoint(_forend);

    ir->loop_terminals.pop();
    return nullptr; // for statement doesn't return any value
}


inline llvm::Value *AST_While_Expression::generate_ir(LLVM_IR *ir)
{
    // here we will need labels for the
    // while condition, while body,
    // and the while end

    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for \'while\' statement.");
    }

    llvm::Function* f = ir->_builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* _whilecond  = llvm::BasicBlock::Create(ir->_context, "whilecond", f);
    llvm::BasicBlock* _body  = llvm::BasicBlock::Create(ir->_context, "whilebody", f);
    llvm::BasicBlock* _whileend = llvm::BasicBlock::Create(ir->_context, "whileend", f);

    // jump to condition block first
    ir->_builder->CreateBr(_whilecond);

    // emit condition
    ir->_builder->SetInsertPoint(_whilecond);
    llvm::Value* _condition = condition->generate_ir(ir);
    if (!_condition) return nullptr;

    _condition = ir->_builder->CreateICmpNE(
        _condition,
        llvm::ConstantInt::get(_condition->getType(), 0),
        "whilecond"
    );

    auto *terminals = new Loop_Terminals;
    terminals->loop_condition = _whilecond;
    terminals->loop_end = _whileend;
    ir->loop_terminals.push(terminals);

    // create conditional branch
    ir->_builder->CreateCondBr(_condition, _body, _whileend);

    // emit body
    _body->insertInto(f);
    ir->_builder->SetInsertPoint(_body);

    bool has_return_expression_in_block = generate_block_ir(ir, block);

    // jump back to condition
    if (!has_return_expression_in_block) ir->_builder->CreateBr(_whilecond);
    _body = ir->_builder->GetInsertBlock();

    // _whileend (for termination)
    _whileend->insertInto(f);
    ir->_builder->SetInsertPoint(_whileend);

    ir->loop_terminals.pop();
    return nullptr; // while statement returns no value
}


inline llvm::Value *AST_Declaration::generate_ir(LLVM_IR *ir)
{
    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for declaration.");
    }

    llvm::Function *f = ir->_builder->GetInsertBlock()->getParent();

    // create alloca at the beginning of the entry block
    llvm::IRBuilder<> tmp_builder(ir->_context);
    llvm::BasicBlock &_entry = f->getEntryBlock();
    tmp_builder.SetInsertPoint(&_entry, _entry.begin());

    llvm::Type *var_type = llvm_type_map(data_type, ir->_context);
    llvm::AllocaInst *_alloca = tmp_builder.CreateAlloca(var_type, nullptr, variable_name);

    // store it in the symbol table
    auto *sym_info = new LLVM_Symbol_Info{ _alloca, var_type };
    ir->llvm_symbol_table.insert(variable_name, sym_info);
    return _alloca;
}


inline llvm::Value *AST_Unary_Expression::generate_ir(LLVM_IR *ir)
{
    switch (op) {
    case TOKEN_NOT: {
	// get a 0 having a type same as val
	llvm::Value* val = expr->generate_ir(ir);
        llvm::Value *zero = llvm::ConstantInt::get(val->getType(), 0);
        return ir->_builder->CreateICmpEQ(val, zero, "nottmp");
    }
    case TOKEN_BIT_NOT: {
	// get an integer with all bits set to 1, of the same type as val
	llvm::Value* val = expr->generate_ir(ir);
        llvm::Value *all_ones = llvm::ConstantInt::get(val->getType(), -1, true);
        return ir->_builder->CreateXor(val, all_ones, "bnot");
    }
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT: {
	// for increments / decrements, the expression
	// must be an lvalue, and we will need the address to it,
	// instead of its direct value.

	if (expr->expr_type != EXPR_IDENT) {
	    throw_ir_error("Cannot increment/decrement a non-lvalue expression).");
	}

        llvm::Value *val_addr = ((AST_Identifier*)expr)->generate_ir_pointer(ir);
	llvm::Value *val = expr->generate_ir(ir);

	if (val && val->getType()->isPointerTy())
            val = ir->_builder->CreateLoad(llvm::dyn_cast<llvm::PointerType>(val->getType()), val);

        int delta = (op == TOKEN_INCREMENT) ? 1 : -1;
        llvm::Value *one = llvm::ConstantInt::get(val->getType(), delta);
        llvm::Value *new_val = ir->_builder->CreateAdd(val, one, "incdec");

        ir->_builder->CreateStore(new_val, val_addr);
	return (is_postfix) ? val : new_val;
    }
    default: throw_ir_error("Invalid unary operator encountered.");
    }
    return nullptr; // added to prevent warnings (should never reach here)
}


inline llvm::Value *AST_Binary_Expression::generate_ir(LLVM_IR *ir)
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

    if (op == TOKEN_AND) return generate_ir__logical_and(left, right, ir);
    if (op == TOKEN_OR) return generate_ir__logical_or(left, right, ir);

    llvm::Value *L = nullptr, *Rval = nullptr;
    if (left) {
	// turns out this is not as simple as it looks.
	// in case of assignments, if the left side is an identifier
	// we don't want the value to be loaded, instead we just want
	// the address of the identifier.

	// so the solution to this is to just take in the pointer in
	// case it is an identifier. then if we need to load it, we
	// can do that manually wherever actually needed.

	L = (left->expr_type == EXPR_IDENT)
	? ((AST_Identifier*)left)->generate_ir_pointer(ir)
	: left->generate_ir(ir);
    }
    if (right) Rval = right->generate_ir(ir);

    if (!L) return Rval;
    else if (!Rval) {
	// we need to return the Lval here
	return (left->expr_type == EXPR_IDENT)
	? left->generate_ir(ir)
	: L;
    }


    /*
    For Assignments:

	Either we could have a normal assignment (=)
	Or, a combination of some binary operation and assignment (like +=, -=, etc.)
    */

    if (op == TOKEN_ASSIGN) {
        if (Rval && Rval->getType()->isPointerTy())
            Rval = ir->_builder->CreateLoad(llvm::dyn_cast<llvm::PointerType>(Rval->getType()), Rval);

        ir->_builder->CreateStore(Rval, L);  // here L must be an address
        return Rval;
    }

    // for compound assignments

    llvm::Value *Lval = left->generate_ir(ir);

    if (Lval && Lval->getType()->isPointerTy())
        Lval = ir->_builder->CreateLoad(llvm::dyn_cast<llvm::PointerType>(Lval->getType()), Lval);
    if (Rval && Rval->getType()->isPointerTy())
        Rval = ir->_builder->CreateLoad(llvm::dyn_cast<llvm::PointerType>(Rval->getType()), Rval);

    llvm::Value *result = nullptr;

    switch (op) {
    case TOKEN_PLUSEQ:     result = ir->_builder->CreateAdd(Lval, Rval, "addtmp"); break;
    case TOKEN_MINUSEQ:    result = ir->_builder->CreateSub(Lval, Rval, "subtmp"); break;
    case TOKEN_MULTIPLYEQ: result = ir->_builder->CreateMul(Lval, Rval, "multmp"); break;
    case TOKEN_DIVIDEEQ:   result = ir->_builder->CreateSDiv(Lval, Rval, "divtmp"); break;
    case TOKEN_MODEQ:      result = ir->_builder->CreateSRem(Lval, Rval, "modtmp"); break;
    case TOKEN_LSHIFT_EQ:  result = ir->_builder->CreateShl(Lval, Rval, "lshtmp"); break;
    case TOKEN_RSHIFT_EQ:  result = ir->_builder->CreateAShr(Lval, Rval, "rshtmp"); break;
    case TOKEN_ANDEQ:      result = ir->_builder->CreateAnd(Lval, Rval, "andtmp"); break;
    case TOKEN_OREQ:       result = ir->_builder->CreateOr(Lval, Rval, "ortmp"); break;
    case TOKEN_BIT_ANDEQ:  result = ir->_builder->CreateAnd(Lval, Rval, "andtmp"); break;
    case TOKEN_BIT_OREQ:   result = ir->_builder->CreateOr(Lval, Rval, "ortmp"); break;
    case TOKEN_XOREQ:      result = ir->_builder->CreateXor(Lval, Rval, "xortmp"); break;
    default: break;
    }

    // store the result back into original L (must be an address)
    if (result) {
	ir->_builder->CreateStore(result, L);
	return result;
    }


    // For other logical / bitwise operations
    // In this case, both sides should be values

    switch (op) {
    case TOKEN_PLUS:      return ir->_builder->CreateAdd(Lval, Rval, "addtmp");
    case TOKEN_MINUS:     return ir->_builder->CreateSub(Lval, Rval, "subtmp");
    case TOKEN_STAR:      return ir->_builder->CreateMul(Lval, Rval, "multmp");
    case TOKEN_DIVIDE:    return ir->_builder->CreateSDiv(Lval, Rval, "divtmp");
    case TOKEN_MOD:       return ir->_builder->CreateSRem(Lval, Rval, "modtmp");
    case TOKEN_LESS:      return ir->_builder->CreateICmpSLT(Lval, Rval, "cmptmp");
    case TOKEN_GREATER:   return ir->_builder->CreateICmpSGT(Lval, Rval, "cmptmp");
    case TOKEN_LESSEQ:    return ir->_builder->CreateICmpSLE(Lval, Rval, "cmptmp");
    case TOKEN_GREATEREQ: return ir->_builder->CreateICmpSGE(Lval, Rval, "cmptmp");
    case TOKEN_EQUAL:     return ir->_builder->CreateICmpEQ(Lval, Rval, "cmptmp");
    case TOKEN_NOTEQ:     return ir->_builder->CreateICmpNE(Lval, Rval, "cmptmp");
    case TOKEN_LSHIFT:    return ir->_builder->CreateShl(Lval, Rval, "lshtmp");
    case TOKEN_RSHIFT:    return ir->_builder->CreateAShr(Lval, Rval, "rshtmp");
    case TOKEN_BIT_OR:    return ir->_builder->CreateOr(Lval, Rval, "ortmp");
    case TOKEN_XOR:       return ir->_builder->CreateXor(Lval, Rval, "xortmp");
    case TOKEN_AMPERSAND: return ir->_builder->CreateAnd(Lval, Rval, "andtmp");
    default: break;
    }

    return nullptr;
}


inline llvm::Value *AST_Function_Call::generate_ir(LLVM_IR *ir)
{
    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for function call.");
    }

    // find the function in the module
    llvm::Function* callee = ir->_module->getFunction(function_name);
    if (!callee) {
	throw_ir_error("Invalid function call.");
    }

    // generate instructions for each argument
    std::vector<llvm::Value*> args;
    for (auto* param : params) {
        llvm::Value* arg_val = param->generate_ir(ir);
        if (!arg_val) return nullptr;
        args.push_back(arg_val);
    }

    // in case the return type is void, we should not return anything
    if (callee->getReturnType()->isVoidTy()) {
        ir->_builder->CreateCall(callee, args);
        return nullptr;
    }

    // emit the call instruction
    return ir->_builder->CreateCall(callee, args, "calltmp");
}


inline llvm::Value *AST_Return_Expression::generate_ir(LLVM_IR *ir)
{
    if (!value) return ir->_builder->CreateRetVoid(); // ret void

    llvm::Value *val = value->generate_ir(ir);
    if (!val) return nullptr;

    // verify function return type is being matched
    llvm::Type *retTy = ir->_builder->GetInsertBlock()->getParent()->getReturnType();
    if (val->getType() != retTy) {
	// add cast if needed
	if (val->getType()->isIntegerTy() && retTy->isIntegerTy()) {
	    val = ir->_builder->CreateIntCast(val, retTy, true, "retcast");
	}
	else {
	    print_ir(ir->_module);
	    throw_ir_error("Return value type does not match the function return type.");
	}
    }

    return ir->_builder->CreateRet(val); // ret <value>
}


inline llvm::Value *AST_Jump_Expression::generate_ir(LLVM_IR *ir)
{
    // we just peek at the top of the loop stack
    // to get to know the label of the condition/end
    // of the loop where we need to jump to.

    if (ir->loop_terminals.empty()) {
	throw_ir_error("\'break\'/\'continue\' cannot be used outside a loop.");
    }

    Loop_Terminals *current = ir->loop_terminals.top();

    switch (jump_type) {
    case J_BREAK: ir->_builder->CreateBr(current->loop_end); break;
    case J_CONTINUE: ir->_builder->CreateBr(current->loop_condition); break;
    default: throw_ir_error("Invalid jump type encountered.");
    }

    if (ir->_builder->GetInsertBlock() == nullptr) {
	throw_ir_error("(FATAL) Cannot find parent IR block for jump statement.");
    }

    llvm::Function* f = ir->_builder->GetInsertBlock()->getParent();
    auto *jumpend = llvm::BasicBlock::Create(ir->_context, "jumpend", f);
    ir->_builder->SetInsertPoint(jumpend);

    return nullptr; // break and continue don't return any value
}


inline llvm::Value *AST_Block_Expression::generate_ir(LLVM_IR *ir)
{
    generate_block_ir(ir, block);
    return nullptr; // scoped-expressions don't return any value
}


llvm::Value *generate_ir__global_declaration(LLVM_IR *ir, AST_Expression *expr)
{
    // either this is a binary expression with the (=) operator,
    // or it is a simple declaration

    switch (expr->expr_type) {
    case EXPR_DECL: {
	auto *decl = (AST_Declaration*)expr;

        llvm::Type *var_type = llvm_type_map(decl->data_type, ir->_context);
        llvm::Constant *init = llvm::Constant::getNullValue(var_type);

        auto *global = new llvm::GlobalVariable(
            *(ir->_module),
            var_type,
            false,
            llvm::GlobalValue::ExternalLinkage,
            init,
            decl->variable_name
        );

	auto *sym_info = new LLVM_Symbol_Info{ global, var_type };
        ir->llvm_symbol_table.insert(decl->variable_name, sym_info);
        return global;
    }
    case EXPR_BINARY: {
	auto *bin = (AST_Binary_Expression*)expr;

        if (bin->op != TOKEN_ASSIGN) {
            throw_ir_error("Global declaration can only be of assignment type.");
        }

        // left branch must be a declaration
        auto *decl = (AST_Declaration*)(bin->left);

        llvm::Type *var_type = llvm_type_map(decl->data_type, ir->_context);
        llvm::Value *right_val = bin->right->generate_ir(ir);
        llvm::Constant *init = llvm::dyn_cast<llvm::Constant>(right_val);
        if (!init) {
            throw_ir_error("Global initializers must be constant expressions.");
        }

        auto *global = new llvm::GlobalVariable(
            *(ir->_module),
            var_type,
            false,
            llvm::GlobalValue::ExternalLinkage,
            init,
            decl->variable_name
        );

	auto *sym_info = new LLVM_Symbol_Info{ global, var_type };
        ir->llvm_symbol_table.insert(decl->variable_name, sym_info);
        return global;
    }
    default: {
	throw_ir_error("Invalid top-level expression encountered.");
    }
    }
    return nullptr;
}


// goes through each top-level expression in the AST
// and runs the IR generation for each of them.
// this emits the llvm IR into the module.
LLVM_IR *emit_llvm_ir(std::vector<AST_Expression*> *ast, const char *file_name)
{
    auto *_context = new llvm::LLVMContext;                // creating a context for this file
    auto *_module = new llvm::Module(file_name, *_context); // container for functions/vars
    auto *_builder = new llvm::IRBuilder<>(*_context);      // helper to generate instructions

    auto *ir = new LLVM_IR(*_context, _builder, _module);

    // if a top-level expression is a non-function
    // then it must be either a declaration, or a binary
    // expression that has a declaration on the left side.
    // these must be treated separately, to emit a global
    // declaration LLVM instruction.

    for (AST_Expression *ast_expr : *ast) {
	if (ast_expr->expr_type != EXPR_FUNC_DEF) {
	    generate_ir__global_declaration(ir, ast_expr);
	} else ast_expr->generate_ir(ir);
    }

    // verify the LLVM IR generated
    llvm::verifyModule(*_module, &llvm::errs());

    return ir;
}


// writes the LLVM IR code to a .ll file
void write_llvm_ir_to_file(const char *llvm_file_name, llvm::Module *_module)
{
    std::error_code EC;
    llvm::raw_fd_ostream output_file(llvm_file_name, EC);
    _module->print(output_file, nullptr);
}
