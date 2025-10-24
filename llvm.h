//
// llvm.h
//

#ifndef __LLVM_H
#define __LLVM_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

/* for running LLVM backend */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/MC/TargetRegistry.h"

/* for cloning modules */
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/MemoryBuffer.h"
//#include "llvm/Transforms/Utils/Cloning.h"

/* for linking */
#include "llvm/Linker/Linker.h"

#endif
