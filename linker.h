//
// linker.h
//

/*

here we handle all the linking logic
required for this compiler. broadly this
involves two kinds of linking:

    (1) Linking LLVM modules into a single
    module under a shared context.

    (2) Linking the runtime for the particular
    operating system, along with the standard
    libraries for this language.

*/

#ifndef LINKER_H
#define LINKER_H

#include "llvm.h"
#include <filesystem>

std::unique_ptr<llvm::Module>
link_modules(std::vector<std::unique_ptr<llvm::Module>> module_list);

void make_executable_from_object(std::string object_file_name);

#endif
