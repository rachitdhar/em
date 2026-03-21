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

#pragma once

#include "llvm.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif


// this will be read from the cached_paths.txt
struct Cached_Paths {
    std::filesystem::path Windows_SDK;
    std::filesystem::path VS_Build_Tools;
};


std::unique_ptr<llvm::Module>
link_modules(std::vector<std::unique_ptr<llvm::Module>> module_list);

void make_executable_from_object(std::string object_file_name);
std::filesystem::path get_compiler_executable_path();
std::string get_include_path();
std::string get_lib_path();
