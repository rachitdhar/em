#define NOB_IMPLEMENTATION
#include "nob.h"

int main() {
    Nob_Cmd cmd = {0};

    nob_cmd_append(&cmd, "clang++");

    nob_cmd_append(&cmd, "lexer.cpp");
    nob_cmd_append(&cmd, "parser.cpp");
    nob_cmd_append(&cmd, "ir_generator.cpp");

    nob_cmd_append(&cmd, "-o");
    nob_cmd_append(&cmd, "compiler.exe");

    // Include paths
    nob_cmd_append(&cmd, "-I");
    nob_cmd_append(&cmd, "D:/softwares/mingw64/include");
    nob_cmd_append(&cmd, "-I");
    nob_cmd_append(&cmd, "D:/github/llvm-project/llvm/include");
    nob_cmd_append(&cmd, "-I");
    nob_cmd_append(&cmd, "D:/github/llvm-build/include");

    // Compiler flags
    nob_cmd_append(&cmd, "-std=c++17");
    nob_cmd_append(&cmd, "-D_FILE_OFFSET_BITS=64");
    nob_cmd_append(&cmd, "-D__STDC_CONSTANT_MACROS");
    nob_cmd_append(&cmd, "-D__STDC_FORMAT_MACROS");
    nob_cmd_append(&cmd, "-D__STDC_LIMIT_MACROS");

    // Library paths
    nob_cmd_append(&cmd, "-L");
    nob_cmd_append(&cmd, "D:/softwares/mingw64/lib");
    nob_cmd_append(&cmd, "-L");
    nob_cmd_append(&cmd, "D:/github/llvm-build/lib");

    // LLVM libraries (order matters)
    nob_cmd_append(&cmd, "-lLLVMIRReader");
    nob_cmd_append(&cmd, "-lLLVMAsmParser");
    nob_cmd_append(&cmd, "-lLLVMCore");
    nob_cmd_append(&cmd, "-lLLVMSupport");
    nob_cmd_append(&cmd, "-lLLVMDemangle");
    nob_cmd_append(&cmd, "-lLLVMBitstreamReader");
    nob_cmd_append(&cmd, "-lLLVMBinaryFormat");
    nob_cmd_append(&cmd, "-lLLVMRemarks");
    nob_cmd_append(&cmd, "-lLLVMTargetParser");

    // Run the command synchronously
    if (!nob_cmd_run_sync(cmd)) {
        return 1; // failed
    }

    return 0; // success
}
