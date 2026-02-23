/* *************************************************

Tests

The list of categories that need to be tested are:

- Basic tokenization
- Function definitions
- Declarations
- Literals
- Function calls
- if-else
- for
- while
- break
- continue
- return
- Assignment
- Binary operations
- Unary operations

Basic rules to follow

I'll try to write at least four kinds of tests
for each category above. Broadly, they can cover
the following ideas:

    1. Basic syntax                   (Positive test)
    2. General uses                   (Positive test)
    3. Complex scenarios / Edge cases (Positive test)
    4. Scenarios that should fail     (Negative test)

************************************************* */

#include <fstream>
#include <stdio.h>
#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <cstdio>

namespace fs = std::filesystem;

// to run the tests
int main()
{
    // Preliminary check: run emc without arguments and check output
    FILE* pipe = _popen("\"d:\\github\\emc\\emc.exe\" 2>&1", "r");
    if (!pipe) {
        std::cout << "\033[31mABORT: emc compiler not detected / displaying undefined behavior. Testing halted.\033[0m" << std::endl;
        return 1;
    }
    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    _pclose(pipe);
    // Remove trailing newline
    if (!output.empty() && output.back() == '\n') output.pop_back();
    if (output.find("ERROR") == std::string::npos) {
        std::cout << "\033[31mABORT: emc compiler not detected / displaying undefined behavior. Testing halted.\033[0m" << std::endl;
        return 1;
    }

    std::vector<std::string> positive_files;
    std::vector<std::string> negative_files;

    // Collect positive test files
    for (const auto& entry : fs::directory_iterator("positive")) {
        if (entry.path().extension() == ".em") {
            positive_files.push_back(entry.path().string());
        }
    }

    // Collect negative test files
    for (const auto& entry : fs::directory_iterator("negative")) {
        if (entry.path().extension() == ".em") {
            negative_files.push_back(entry.path().string());
        }
    }

    // Run positive tests
    std::cout << "Running Positive Test Cases:" << std::endl;
    std::cout << "=============================" << std::endl;
    for (const auto& file : positive_files) {
        std::string command = "d:/github/emc/emc.exe " + file + " >nul 2>>test_logs.txt";
        int result = system(command.c_str());
        std::string status = (result == 0) ? "passed" : "failed";
        std::cout << std::left << std::setw(50) << file << (status == "passed" ? "\033[32mpassed\033[0m" : "\033[31mfailed\033[0m") << std::endl;
    }

    // Run negative tests
    std::cout << "\nRunning Negative Test Cases:" << std::endl;
    std::cout << "=============================" << std::endl;
    for (const auto& file : negative_files) {
        std::string command = "d:/github/emc/emc.exe " + file + " >nul 2>>test_logs.txt";
        int result = system(command.c_str());
        std::string status = (result != 0) ? "passed" : "failed";
        std::cout << std::left << std::setw(50) << file << (status == "passed" ? "\033[32mpassed\033[0m" : "\033[31mfailed\033[0m") << std::endl;
    }

    return 0;
}
