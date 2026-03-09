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

std::string get_lld_link_path()
{
    std::filesystem::path exe_path = get_compiler_executable_path();
    auto bin_path = exe_path.parent_path();
    auto lld_link_path = bin_path / "lld-link.exe";
    return lld_link_path.string();
}

// here we will create an executable for the .o file
void make_executable_from_object(std::string object_file_name)
{
#ifdef _WIN32

    HKEY hKey;
    char sdkroot[MAX_PATH];
    DWORD size = sizeof(sdkroot);

    // Read Windows SDK root from registry
    if (RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
        0,
        KEY_READ,
        &hKey) != ERROR_SUCCESS)
    {
        fprintf(stderr, "LINKER ERROR: Failed to create exe: Cannot open registry key\n");
        exit(1);
    }

    if (RegQueryValueExA(
        hKey,
        "KitsRoot10",
        NULL,
        NULL,
        (LPBYTE)sdkroot,
        &size) != ERROR_SUCCESS)
    {
        fprintf(stderr, "LINKER ERROR: Failed to create exe: Windows 10 SDK not found\n");
        RegCloseKey(hKey);
        exit(1);
    }

    RegCloseKey(hKey);

    std::filesystem::path sdkRoot = sdkroot;

    if (sdkRoot.filename() == "")
        sdkRoot = sdkRoot.parent_path();

    std::filesystem::path libBase = sdkRoot / "Lib";

    // Find latest SDK version
    std::string latestVersion;

    for (auto& entry : std::filesystem::directory_iterator(libBase))
    {
        if (!entry.is_directory()) continue;

        std::string ver = entry.path().filename().string();
        if (latestVersion.empty() || ver > latestVersion) latestVersion = ver;
    }

    if (latestVersion.empty())
    {
        fprintf(stderr, "LINKER ERROR: Failed to create exe: No SDK versions found\n");
        exit(1);
    }

    std::filesystem::path libPath = libBase / latestVersion / "um" / "x64";
    std::filesystem::path kernel32 = libPath / "kernel32.lib";

    if (!std::filesystem::exists(kernel32))
    {
        fprintf(stderr, "LINKER ERROR: Failed to create exe: kernel32.lib not found in %s\n", libPath.string().c_str());
        exit(1);
    }

    // Construct paths
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path obj = cwd / (object_file_name + ".o");
    std::filesystem::path exe = object_file_name + ".exe";

    std::string lld_link_path = get_lld_link_path();

    std::string command =
        lld_link_path + " \"" + obj.string() + "\" "
        "/LIBPATH:\"" + libPath.string() + "\" "
        "kernel32.lib "
        "/SUBSYSTEM:CONSOLE "
        "/ENTRY:_start "
        "/NODEFAULTLIB "
        "/OUT:\"" + exe.string() + "\"";

    // Run linker
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessA(
        lld_link_path.c_str(),
        command.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi))
    {
        fprintf(stderr, "LINKER ERROR: Failed to create exe: Failed to run lld-link\n");
        exit(1);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0)
    {
        fprintf(stderr, "ERROR: Linking failed\n");
        exit(1);
    }
#endif
}