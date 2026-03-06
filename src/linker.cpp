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

// obtain the path to the compiler .exe file (which is in the bin/ folder).
// this is crucial to locate include/ headers and lib/ files.
std::filesystem::path get_compiler_executable_path()
{
#if defined(_WIN32)

    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(std::string(buffer, len));

#elif defined(__APPLE__)

    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size);
    std::string buffer(size, '\0');

    if (_NSGetExecutablePath(buffer.data(), &size) == 0)
        return std::filesystem::canonical(buffer);

    return {};

#elif defined(__linux__)

    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

    if (len != -1)
    {
        buffer[len] = '\0';
        return std::filesystem::path(buffer);
    }

    return {};

#else
#error Unsupported platform
#endif
}

std::string get_include_path()
{
    std::filesystem::path exe_path = get_compiler_executable_path();
    auto project_path = exe_path.parent_path().parent_path();
    auto include_path = project_path / "include";
    return include_path.string() + '\\';
}

std::string get_lib_path()
{
    std::filesystem::path exe_path = get_compiler_executable_path();
    auto project_path = exe_path.parent_path().parent_path();
    auto lib_path = project_path / "lib";
    return lib_path.string() + '\\';
}
