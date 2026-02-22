//
// llvm.h
//

#ifndef __LLVM_H
#define __LLVM_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

/* for running LLVM backend */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

/* for cloning modules */
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/MemoryBuffer.h"

/* for linking */
#include "llvm/Linker/Linker.h"

// This function is needed for a very particular reason. The thing is that if we
// compile multiple files, we would get multiple different modules for each
// such file, and then we would have to link them into a single module that is
// then sent to the LLVM backend (to get the asm or object file). Now, we cannot
// have different contexts, at least in the naive sense of it. Because if each
// module is associated with a different context, then the linking step will
// simply fail. What we require is a single shared context.

// All this is fine. But here comes the real problem. To optimize the
// compilation process for performance, we would like to compile all files using
// multiple threads in parallel. This is however a serious problem, because if
// we keep a single shared LLVM context with multiple threads working on it, the
// modules again don't come out right. LLVM objects are in general not thread
// safe. What this means is that we cannot keep a shared context. It is
// necessary to have separate contexts for all modules.

// But how can we resolve these two problems - threaded compilation and linking
// - if they have opposing needs? The only way (that I know of) is to (1) create
// modules with independent contexts (this step can be done in parallel with
// threads); (2) move/clone each of these modules into a single shared context
// AFTER all modules have been created; and (3) then link all these new modules
// (which are now under a single shared context).

// The function below helps perform step (2) in this process. Based on some
// reading it seems that earlier LLVM had an inbuilt llvm::CloneModule function
// that could move modules to a different context, but in LLVM 21 (which I am
// using for this project), the CloneModule function does not have any variant
// that moves a module to a context other than its original context.

// DESCRIPTION:
//     clone module into a destination context by writing bitcode to an
//     in-memory buffer, then parsing that buffer back into dest_context.
inline std::unique_ptr<llvm::Module>
move_module_to_context(llvm::Module *mod, llvm::LLVMContext &new_context) {
    // write bitcode into a SmallVector<char> buffer
    llvm::SmallVector<char, 0> buffer;
    llvm::raw_svector_ostream os(buffer);

    llvm::WriteBitcodeToFile(*mod, os);

    llvm::StringRef dataRef(buffer.data(), buffer.size());
    llvm::MemoryBufferRef memRef(dataRef, mod->getModuleIdentifier());

    // parse into new_context
    llvm::Expected<std::unique_ptr<llvm::Module>> module_or_error =
        llvm::parseBitcodeFile(memRef, new_context);

    if (!module_or_error) {
        llvm::errs() << "ERROR: Module cloning failed.";
        exit(1);
    }

    std::unique_ptr<llvm::Module> new_module = std::move(*module_or_error);

    new_module->setTargetTriple(mod->getTargetTriple());
    new_module->setDataLayout(mod->getDataLayout());

    // verify cloned module
    if (llvm::verifyModule(*new_module, &llvm::errs())) {
        llvm::errs() << "ERROR: Cloned module verification failed.";
        exit(1);
    }
    return new_module;
}

// to read a .bc file (LLVM bitcode) and create a module from it
inline std::unique_ptr<llvm::Module>
get_module_from_bitcode(const std::string &filename,
                        llvm::LLVMContext &context) {
    // open the bitcode file as a memory buffer
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer_or_error =
        llvm::MemoryBuffer::getFile(filename);

    if (!buffer_or_error) {
        llvm::errs() << "Error: could not open bitcode file '" << filename
                     << "': " << buffer_or_error.getError().message() << "\n";
        exit(1);
    }

    // parse the bitcode buffer into a module
    llvm::Expected<std::unique_ptr<llvm::Module>> module_or_error =
        llvm::parseBitcodeFile(buffer_or_error->get()->getMemBufferRef(),
                               context);

    if (!module_or_error) {
        llvm::errs() << "Error: failed to parse bitcode file '" << filename
                     << "': ";
        llvm::handleAllErrors(module_or_error.takeError(),
                              [](const llvm::ErrorInfoBase &EIB) {
                                  llvm::errs() << EIB.message() << "\n";
                              });
        exit(1);
    }
    return std::move(*module_or_error);
}

#endif
