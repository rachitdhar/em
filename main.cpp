//
// main.cpp
//

/*

This is the entry point for the compiler.

Here we shall handle the actual compilation process,
from lexical analysis, to parsing, ir generation, and
finally the production of the executable for the target
machine.

Additionally, we would like to handle any flags that are
passed.

*/

#include "ir_generator.h"
#include <chrono>



enum Output_File_Type { OBJ, ASM };


// to generate the executable / assembly file for the particular target
void run_llvm_backend(llvm::Module *_module, const std::string &out_file_name, Output_File_Type output_file_type) {
    // initialize all targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(target_triple);
    _module->setTargetTriple(triple);

    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        llvm::errs() << error;
        return;
    }

    std::string cpu = "generic";
    std::string features = "";

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    llvm::TargetMachine *target_machine =
        target->createTargetMachine(target_triple, cpu, features, opt, RM);

    _module->setDataLayout(target_machine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(out_file_name, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "ERROR: Could not open file: " << EC.message();
        return;
    }

    llvm::legacy::PassManager pass;

    llvm::CodeGenFileType file_type;
    switch (output_file_type) {
    case OBJ: file_type = llvm::CodeGenFileType::ObjectFile; break;
    case ASM: file_type = llvm::CodeGenFileType::AssemblyFile; break;
    }

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        llvm::errs() << "ERROR: Target machine can't emit output file";
        return;
    }

    pass.run(*_module);
    dest.flush();
}


// displays the frontend, backend, and total elapsed times.
void print_benchmark_metrics(
    std::chrono::time_point<std::chrono::high_resolution_clock> frontend_start,
    std::chrono::time_point<std::chrono::high_resolution_clock> frontend_end,
    std::chrono::time_point<std::chrono::high_resolution_clock> backend_end
) {
    // calculating the elapsed time duration in seconds
    std::chrono::duration<double> frontend_elapsed_time = frontend_end - frontend_start;
    std::chrono::duration<double> backend_elapsed_time = backend_end - frontend_end;

    printf("\n         Performance metrics\n");
    printf("-------------------------------------\n");
    printf("Frontend time elapsed: \t%.6f sec\n", frontend_elapsed_time.count());
    printf("Backend time elapsed: \t%.6f sec\n", backend_elapsed_time.count());

    double total_time = frontend_elapsed_time.count() + backend_elapsed_time.count();
    printf("Total execution time: \t%.6f sec\n", total_time);
}


// check the extension of a file (ext is to be passed without a dot)
int has_extension(const char *file_name, const char *ext) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) return 0;
    return strcmp(dot + 1, ext) == 0;
}


int main(int argc, char **argv)
{
    // keeping track of the execution time for benchmarking metrics
    auto frontend_start = std::chrono::high_resolution_clock::now();
    /*
    The basic compilation command should be something like:

	<compiler> "<PATH_TO_FILE>" ...
	                             ^ flags (optional)
    */

    if (argc < 2) {
	fprintf(stderr, "ERROR: Provide the path of the file to be compiled.");
	exit(1);
    }

    const char *file_name = argv[1];
    if (!has_extension(file_name, "van")) {
	fprintf(stderr, "ERROR: Invalid file type. File must have a .van extension.");
	exit(1);
    }

    Lexer *lexer = perform_lexical_analysis(file_name);
    auto *ast = parse_tokens(lexer);

    llvm::LLVMContext _context;                // holds global LLVM state
    llvm::Module _module(lexer->file_name.c_str(), _context); // container for functions/vars
    llvm::IRBuilder<> _builder(_context);      // helper to generate instructions

    emit_llvm_ir(ast, _context, &_builder, &_module);
    auto frontend_end = std::chrono::high_resolution_clock::now();


    bool show_benchmarking_metrics = false;
    bool make_output_file = true;
    Output_File_Type output_file_type = OBJ;

    // handle the compiler flags (if any are provided by the user)
    for (int i = 2; i < argc; i++) {
	if (strcmp(argv[i], "-pout") == 0) print_ast(ast);
	else if (strcmp(argv[i], "-llout") == 0) print_ir(&_module);
	else if (strcmp(argv[i], "-ll") == 0) {
	    std::string llvm_file_name = lexer->file_name + ".ll";
	    write_llvm_ir_to_file(llvm_file_name.c_str(), &_module);
	    make_output_file = false;
	}
	else if (strcmp(argv[i], "-asm") == 0) output_file_type = ASM;
	else if (strcmp(argv[i], "-benchmark") == 0) show_benchmarking_metrics = true;
    }

    if (make_output_file) {
	std::string file_extension = output_file_type == OBJ ? ".o" : ".s";
	std::string output_file_name = lexer->file_name + file_extension;
	run_llvm_backend(&_module, output_file_name, output_file_type);
    }
    auto backend_end = std::chrono::high_resolution_clock::now();

    if (show_benchmarking_metrics) {
	print_benchmark_metrics(frontend_start, frontend_end, backend_end);
    }

    return 0;
}
