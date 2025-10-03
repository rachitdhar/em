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
// apparently, we would need to store both the value as well
// as the type (since LLVM only keeps a pointer to the allocated
// memory, and does not seem to maintain type info).

struct LLVM_Symbol_Info {
    llvm::Value *val;
    llvm::Type *type;
};

std::unordered_map<std::string, LLVM_Symbol_Info*> llvm_symbol_table;


// defining a stack to store the pairs <LOOP_CONDITION, LOOP_END>
// whenever we encounter a while/for loop. this is needed
// to be able to jump to those locations when a break/continue
// statement is encountered.

struct Loop_Terminals {
    llvm::BasicBlock *loop_condition;
    llvm::BasicBlock *loop_end;
};

std::stack<Loop_Terminals*> loop_terminals;


inline llvm::Type *llvm_type_map(const Data_Type type, llvm::LLVMContext& _context)
{
    switch (type) {
        case T_INT:    return llvm::Type::getInt32Ty(_context);
        case T_FLOAT:  return llvm::Type::getFloatTy(_context);
        case T_BOOL:   return llvm::Type::getInt1Ty(_context);
        case T_CHAR:   return llvm::Type::getInt8Ty(_context);
        case T_STRING: return llvm::Type::getInt8Ty(_context)->getPointerTo();
        case T_VOID:   return llvm::Type::getVoidTy(_context);
        default:
            return nullptr;
    }
}


// to print an error message, for errors that occur
// during the LLVM IR generation process.
inline void throw_ir_error(const char *message)
{
    fprintf(stderr, "IR ERROR: %s", message);
    exit(1);
}


// For Debugging only
// prints the llvm ir that has been emitted till now
inline void print_ir(llvm::Module *_module)
{
    _module->print(llvm::outs(), nullptr);
}


// cast some llvm value to bool (if possible, else throw an error)
inline llvm::Value *cast_llvm_value_to_bool(
    llvm::Value *val,
    llvm::LLVMContext& _context,
    llvm::IRBuilder<> *_builder
) {
    if (val->getType()->isIntegerTy(1)) return val; // already bool
    if (val->getType()->isIntegerTy()) {
        llvm::Value *zero = llvm::ConstantInt::get(val->getType(), 0);
        return _builder->CreateICmpNE(val, zero, "tobool");
    }
    if (val->getType()->isFloatingPointTy()) {
	llvm::Value *zero = llvm::ConstantFP::get(val->getType(), 0.0);
        return _builder->CreateFCmpONE(val, zero, "tobool");
    }
    if (val->getType()->isPointerTy()) {
        return _builder->CreateICmpNE(
            val,
            llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(val->getType())
            ),
            "tobool"
        );
    }
    throw_ir_error("Non-integer type in logical expression.");
}


inline llvm::Value* generate_ir__logical_and(
    AST_Expression* left,
    AST_Expression* right,
    llvm::LLVMContext& _context,
    llvm::IRBuilder<> *_builder,
    llvm::Module *_module
) {
    llvm::BasicBlock *current_block = _builder->GetInsertBlock();
    llvm::Function* f = current_block->getParent();

    llvm::BasicBlock* andright = llvm::BasicBlock::Create(_context, "andright", f);
    llvm::BasicBlock* andend = llvm::BasicBlock::Create(_context, "andend", f);

    // evaluate left
    llvm::Value* L = left->generate_ir(_context, _builder, _module);
    if (!L) return nullptr;

    L = cast_llvm_value_to_bool(L, _context, _builder);
    _builder->CreateCondBr(L, andright, andend);

    // right block
    _builder->SetInsertPoint(andright);
    llvm::Value* R = right->generate_ir(_context, _builder, _module);
    if (!R) return nullptr;

    R = cast_llvm_value_to_bool(R, _context, _builder);
    _builder->CreateBr(andend);

    // end block
    _builder->SetInsertPoint(andend);
    llvm::PHINode* phi_node = _builder->CreatePHI(llvm::Type::getInt1Ty(_context), 2, "andtmp");
    phi_node->addIncoming(llvm::ConstantInt::getFalse(_context), current_block);
    phi_node->addIncoming(R, andright);

    return phi_node;
}


inline llvm::Value* generate_ir__logical_or(
    AST_Expression* left,
    AST_Expression* right,
    llvm::LLVMContext& _context,
    llvm::IRBuilder<> *_builder,
    llvm::Module *_module
) {
    llvm::BasicBlock *current_block = _builder->GetInsertBlock();
    llvm::Function* f = current_block->getParent();

    llvm::BasicBlock* orright = llvm::BasicBlock::Create(_context, "orright", f);
    llvm::BasicBlock* orend = llvm::BasicBlock::Create(_context, "orend", f);

    // evaluate left
    llvm::Value* L = left->generate_ir(_context, _builder, _module);
    if (!L) return nullptr;

    L = cast_llvm_value_to_bool(L, _context, _builder);
    _builder->CreateCondBr(L, orend, orright);

    // right block
    _builder->SetInsertPoint(orright);
    llvm::Value* R = right->generate_ir(_context, _builder, _module);
    if (!R) return nullptr;

    R = cast_llvm_value_to_bool(R, _context, _builder);
    _builder->CreateBr(orend);

    // end block
    _builder->SetInsertPoint(orend);
    llvm::PHINode* phi_node = _builder->CreatePHI(llvm::Type::getInt1Ty(_context), 2, "ortmp");
    phi_node->addIncoming(llvm::ConstantInt::getTrue(_context), current_block);
    phi_node->addIncoming(R, orright);

    return phi_node;
}


// executes generate_ir for each expression inside
// the block, and returns whether or not a return was
// one of the expressions encountered.
inline bool generate_block_ir(
    llvm::LLVMContext& _context,
    llvm::IRBuilder<> *_builder,
    llvm::Module *_module,
    std::vector<AST_Expression*>& block
) {
    for (auto* expr : block) {
	if (expr) {
	    expr->generate_ir(_context, _builder, _module);

	    // once return/jump expression is encountered
	    // the expressions after it can be ignored.
	    if (expr->expr_type == EXPR_RETURN) return true;
	    if (expr->expr_type == EXPR_JUMP) return false;
	}
    }
    return false;
}

#endif
