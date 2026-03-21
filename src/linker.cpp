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

std::string get_cached_paths_path()
{
    std::filesystem::path exe_path = get_compiler_executable_path();
    auto project_path = exe_path.parent_path().parent_path();
    auto cached_paths_path = project_path / "cached_paths.txt";
    return cached_paths_path.string();
}


// finds the path of the Windows SDK lib folder, and returns it.
// this is needed for libs like kernel32 and ucrt.
std::filesystem::path get_windows_sdk_path()
{
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

    return (libBase / latestVersion);
}


// finds the path of lib folder in VS Build tools, and returns it.
// this is needed for libs like vcruntime, msvcrt, libcmt.
std::filesystem::path get_vs_build_tools_path()
{
    std::filesystem::path vsRoot = "C:\\Program Files\\Microsoft Visual Studio";
    if (!std::filesystem::exists(vsRoot) || !std::filesystem::is_directory(vsRoot)) {
        fprintf(stderr, "Visual Studio root directory not found: %s\n", vsRoot.string().c_str());
        exit(1);
    }

    // Find any valid Visual Studio version folder
    std::filesystem::path vsVersion;

    for (auto& entry : std::filesystem::directory_iterator(vsRoot)) {
        if (!entry.is_directory()) continue;

        // Check if edition folder exists inside this version
        std::filesystem::path editionFolder;
        const char* editions[] = { "BuildTools", "Community", "Professional", "Enterprise" };
        for (const char* ed : editions) {
            std::filesystem::path candidate = entry.path() / ed;
            if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
                editionFolder = candidate;
                break;
            }
        }
        if (editionFolder.empty()) continue;

        // Found a version with a valid edition
        vsVersion = editionFolder;
	break;
    }

    if (vsVersion.empty()) {
        fprintf(stderr, "No Visual Studio versions found in: %s\n", vsRoot.string().c_str());
        exit(1);
    }

    std::filesystem::path vcToolsPath = vsRoot / vsVersion / "VC" / "Tools" / "MSVC";
    if (!std::filesystem::exists(vcToolsPath) || !std::filesystem::is_directory(vcToolsPath)) {
        fprintf(stderr, "VC Tools directory not found: %s\n", vcToolsPath.string().c_str());
        exit(1);
    }

    // Find the latest MSVC version
    std::filesystem::path latestMSVC;
    for (auto& entry : std::filesystem::directory_iterator(vcToolsPath)) {
        if (!entry.is_directory()) continue;
        if (latestMSVC.empty() || entry.path().filename() > latestMSVC.filename()) {
            latestMSVC = entry.path();
        }
    }

    if (latestMSVC.empty()) {
        fprintf(stderr, "No MSVC versions found in: %s\n", vcToolsPath.string().c_str());
        exit(1);
    }

    std::filesystem::path libPath = latestMSVC / "lib" / "x64";
    if (!std::filesystem::exists(libPath) || !std::filesystem::is_directory(libPath)) {
        fprintf(stderr, "Lib directory not found: %s\n", libPath.string().c_str());
        exit(1);
    }

    return libPath;
}


// reads the cached_paths.txt and writes the paths into a struct
void read_cached_paths(Cached_Paths &paths) {
    std::filesystem::path filePath = get_cached_paths_path();
    std::ifstream inFile(filePath);
    if (!inFile) {
        // file does not exist, so create a blank file
        std::ofstream outFile(filePath);
        if (!outFile) {
            fprintf(stderr, "Failed to create file: %s\n", filePath.string().c_str());
            exit(1);
        }
        outFile.close();
    }

    std::string line;
    while (std::getline(inFile, line)) {
        if (line.empty()) continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        if (key == "WINDOWS_SDK_LIB") {
            paths.Windows_SDK = value;
        } else if (key == "VS_BUILD_TOOLS_LIB") {
            paths.VS_Build_Tools = value;
        }
    }
}


// writes the Cached_Paths struct into cached_paths.txt
void write_cached_paths(const Cached_Paths& paths) {
    std::filesystem::path filePath = get_cached_paths_path();
    std::ofstream outFile(filePath);
    if (!outFile) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filePath.string().c_str());
        exit(1);
    }

    outFile << "WINDOWS_SDK_LIB=" << paths.Windows_SDK.string() << "\n";
    outFile << "VS_BUILD_TOOLS_LIB=" << paths.VS_Build_Tools.string() << "\n";
}


// here we will create an executable for the .o file
void make_executable_from_object(std::string object_file_name)
{
#ifdef _WIN32

    // read the cached paths first
    Cached_Paths paths;
    read_cached_paths(paths);

    if (paths.Windows_SDK.empty() || paths.VS_Build_Tools.empty()) {
	paths.Windows_SDK = get_windows_sdk_path();
	paths.VS_Build_Tools = get_vs_build_tools_path();

	// cache them for future
	write_cached_paths(paths);
    }

    // get paths to libs from Windows SDK
    std::filesystem::path sdkLibPath = paths.Windows_SDK;
    std::filesystem::path umLibPath = sdkLibPath / "um" / "x64";
    std::filesystem::path ucrtLibPath = sdkLibPath / "ucrt" / "x64";

    std::filesystem::path kernel32 = umLibPath / "kernel32.lib";
    std::filesystem::path ucrt = ucrtLibPath / "ucrt.lib";

    if (!std::filesystem::exists(kernel32)) {
	fprintf(stderr, "LINKER ERROR: kernel32.lib not found\n");
	exit(1);
    }

    if (!std::filesystem::exists(ucrt)) {
	fprintf(stderr, "LINKER ERROR: ucrt.lib not found\n");
	exit(1);
    }

    // get paths to libs from VS Build Tools
    std::filesystem::path vsBuildToolsLibPath = paths.VS_Build_Tools;
    std::filesystem::path vcruntime = vsBuildToolsLibPath / "vcruntime.lib";
    std::filesystem::path msvcrt = vsBuildToolsLibPath / "msvcrt.lib";
    std::filesystem::path libcmt = vsBuildToolsLibPath / "libcmt.lib";

    if (!std::filesystem::exists(vcruntime)) {
	fprintf(stderr, "LINKER ERROR: vcruntime.lib not found\n");
	exit(1);
    }

    if (!std::filesystem::exists(msvcrt)) {
	fprintf(stderr, "LINKER ERROR: msvcrt.lib not found\n");
	exit(1);
    }

    if (!std::filesystem::exists(libcmt)) {
	fprintf(stderr, "LINKER ERROR: libcmt.lib not found\n");
	exit(1);
    }

    // Construct paths
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path obj = cwd / (object_file_name + ".o");
    std::filesystem::path exe = object_file_name + ".exe";

    std::string lld_link_path = get_lld_link_path();

    std::string command =
    lld_link_path + " \"" + obj.string() + "\" "
    "/LIBPATH:\"" + umLibPath.string() + "\" "
    "/LIBPATH:\"" + ucrtLibPath.string() + "\" "
    "/LIBPATH:\"" + vsBuildToolsLibPath.string() + "\" "
    "kernel32.lib "
    "ucrt.lib "
    "vcruntime.lib "
    "msvcrt.lib "
    "libcmt.lib "
    "/SUBSYSTEM:CONSOLE "
    "/ENTRY:mainCRTStartup "
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
