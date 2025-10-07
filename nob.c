#define NOB_IMPLEMENTATION

#include "nob.h"
#include <string.h>


int main(int argc, char** argv) {
    int compile_with_debug_symbols = 0;
    if (argc > 1 && strcmp(argv[1], "-debug") == 0) {
	compile_with_debug_symbols = 1;
    }

    Nob_Cmd cmd = {0};

    nob_cmd_append(&cmd, "clang++");

    // to disable warnings
    nob_cmd_append(&cmd, "-w");

    // to compile with debug symbols (if demanded)
    if (compile_with_debug_symbols) {
	nob_cmd_append(&cmd, "-g");
    }

    nob_cmd_append(&cmd, "lexer.cpp");
    nob_cmd_append(&cmd, "parser.cpp");
    nob_cmd_append(&cmd, "ir_generator.cpp");
    nob_cmd_append(&cmd, "main.cpp");

    nob_cmd_append(&cmd, "-o");
    nob_cmd_append(&cmd, "em");

    // include paths
    nob_cmd_append(&cmd, "-I");
    nob_cmd_append(&cmd, "D:/softwares/msys64/mingw64/include");

    // compiler flags
    nob_cmd_append(&cmd, "-std=c++17");
    nob_cmd_append(&cmd, "-fno-exceptions");
    nob_cmd_append(&cmd, "-funwind-tables");
    nob_cmd_append(&cmd, "-DEXPERIMENTAL_KEY_INSTRUCTIONS");
    nob_cmd_append(&cmd, "-D_FILE_OFFSET_BITS=64");
    nob_cmd_append(&cmd, "-D__STDC_CONSTANT_MACROS");
    nob_cmd_append(&cmd, "-D__STDC_FORMAT_MACROS");
    nob_cmd_append(&cmd, "-D__STDC_LIMIT_MACROS");

    // library paths
    nob_cmd_append(&cmd, "-L");
    nob_cmd_append(&cmd, "D:/softwares/msys64/mingw64/lib");

    // LLVM libraries
    nob_cmd_append(&cmd, "-lLLVM-21");

    // run the command synchronously
    if (!nob_cmd_run_sync(cmd)) {
        return 1; // failed
    }

    return 0; // success
}
