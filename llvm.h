//
// llvm.h
//

/*

This is the file where we define the things we need to generate the
LLVM intermediate representation (IR).

First we have to define the context, module and builder. These are
used in the codegen() function in each AST expression struct,
to help describe what IR instructions are meant to be generated.

*/

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"



llvm::LLVMContext _context;                   // holds global LLVM state
llvm::Module _module("_module", _context);    // container for functions/vars
llvm::IRBuilder<> _builder(_context);         // helper to generate instructions
