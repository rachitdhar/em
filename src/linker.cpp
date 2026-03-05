//
// linker.cpp
//

#include "linker.h"


// to link all the LLVM modules
std::unique_ptr<llvm::Module>
link_modules(std::vector<std::unique_ptr<llvm::Module>> module_list) {
    if (module_list.empty()) {
        fprintf(stderr, "LINKER ERROR: No modules found.\n");
        exit(1);
    }

    std::unique_ptr<llvm::Module> linked_module = std::move(module_list[0]);
    module_list[0] = nullptr;
    llvm::Linker linker(*linked_module);

    for (size_t i = 1; i < module_list.size(); ++i) {
        if (!module_list[i])
            continue;
        if (linker.linkInModule(std::move(module_list[i]))) {
            fprintf(stderr, "LINKER ERROR: Failed to link module %zu.\n", i);
            exit(1);
        }
        module_list[i] = nullptr;
    }
    return linked_module;
}

// here we will create an executable for the .o file
void make_executable_from_object(std::string object_file_name)
{
#ifdef _WIN32
    // this needs to replaced later
    system(("win_make_exe.bat \"" + std::filesystem::current_path().string() + "\" " + object_file_name).c_str());
#endif
}
