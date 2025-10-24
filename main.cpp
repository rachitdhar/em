/*

   Copyright 2025 Rachit Dhar

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

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

For the compilation of multiple files, we would make use
of threads to run the compilation process in parallel.

*/

#include "ir_generator.h"
#include <chrono>
#include <thread>
#include <mutex>

#define LANGUAGE_FILE_EXTENSION "em"



enum Output_File_Type { OBJ, ASM, LL };


struct Flag_Settings
{
    bool print_ast = false;
    bool print_ir = false;
    Output_File_Type output_file_type = OBJ;
    std::string cpu_type;
    std::string output_file_name = "out";
};


struct Compilation_Metrics
{
    int total_lines = 0;
    double frontend_time = 0;
    double backend_time = 0;
    double total_time = 0;
};


const std::string cpu_to_target[][2] = {
    /* Windows/Linux x86 systems */
    {"x86-64", "x86_64-unknown-linux-gnu"},

    /* Embedded / microcontrollers (ARM 32-bit) */
    {"cortex-m3", "armv7m-none-eabi"},
    {"cortex-m4", "armv7em-none-eabi"},
    {"cortex-m7", "armv7em-none-eabi"},

    /* Raspberry Pi / ARM 64-bit */
    {"cortex-a7", "armv7a-unknown-linux-gnueabihf"},  // Pi 2
    {"cortex-a53", "aarch64-unknown-linux-gnu"},      // Pi 3
    {"cortex-a72", "aarch64-unknown-linux-gnu"},      // Pi 4

    /* Modern phones */
    {"cortex-a76", "aarch64-unknown-linux-gnu"},
    {"cortex-a78", "aarch64-unknown-linux-gnu"},
    {"cortex-x1", "aarch64-unknown-linux-gnu"},

    /* Apple */
    {"apple-m1", "arm64-apple-darwin"},
    {"apple-m2", "arm64-apple-darwin"},

    /* Cloud ARM servers */
    {"neoverse-n1", "aarch64-unknown-linux-gnu"},
    {"neoverse-v1", "aarch64-unknown-linux-gnu"},
    {"neoverse-n2", "aarch64-unknown-linux-gnu"}
};

const int NUM_CPU_TYPES = sizeof(cpu_to_target) / sizeof(cpu_to_target[0]);



// to generate the executable / assembly file for the particular target
void run_llvm_backend(
    llvm::Module *_module,
    const std::string &out_file_name,
    Output_File_Type output_file_type,
    std::string cpu_type,
    std::string target_triple
) {
    // initialize all targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    if (target_triple == "") target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(target_triple);
    _module->setTargetTriple(triple);

    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        llvm::errs() << error;
        return;
    }

    std::string cpu = cpu_type;
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


// to link all the LLVM modules
std::unique_ptr<llvm::Module> link_modules(std::vector<std::unique_ptr<llvm::Module>> module_list)
{
    if (module_list.empty()) {
	fprintf(stderr, "LINKER ERROR: No modules found.\n");
        exit(1);
    }

    std::unique_ptr<llvm::Module> linked_module = std::move(module_list[0]);
    module_list[0] = nullptr;
    llvm::Linker linker(*linked_module);

    for (size_t i = 1; i < module_list.size(); ++i) {
	if (!module_list[i]) continue;
        if (linker.linkInModule(std::move(module_list[i]))) {
            fprintf(stderr, "LINKER ERROR: Failed to link module %zu.\n", i);
            exit(1);
        }
	module_list[i] = nullptr;
    }
    return linked_module;
}


// displays the total lines, and the frontend, backend, and total elapsed times.
void print_benchmark_metrics(Compilation_Metrics *metrics)
{
    printf("\n         Performance metrics\n");
    printf("-------------------------------------\n");
    printf("Total lines of code: \t%d lines\n", metrics->total_lines);
    printf("Frontend time elapsed: \t%.6f sec\n", metrics->frontend_time);
    printf("Backend time elapsed: \t%.6f sec\n", metrics->backend_time);
    printf("Total execution time: \t%.6f sec\n", metrics->total_time);
}


// check the extension of a file (ext is to be passed without a dot)
int has_extension(const char *file_name, const char *ext) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) return 0;
    return strcmp(dot + 1, ext) == 0;
}


// perform the frontend compilation process for a file, and returns its LLVM module
// also updates the overall compilation metrics as per the metrics for this file.
void compile(
    const char *file_name,
    Flag_Settings *flag_settings,
    bool *entry_point_found,
    std::chrono::time_point<std::chrono::high_resolution_clock> frontend_start,
    Compilation_Metrics *metrics,
    std::mutex *metrics_mutex,
    std::vector<std::unique_ptr<llvm::Module>> *module_list
) {
    if (!has_extension(file_name, LANGUAGE_FILE_EXTENSION)) {
	fprintf(stderr, "ERROR: Invalid file type (%s). File must have a .%s extension.", file_name, LANGUAGE_FILE_EXTENSION);
	exit(1);
    }

    Lexer *lexer = perform_lexical_analysis(file_name);
    auto *ast = parse_tokens(lexer);
    LLVM_IR *ir = emit_llvm_ir(ast, lexer->file_name.c_str());

    if (lexer->entry_point_found) {
	if (*entry_point_found) {
	    fprintf(stderr, "ERROR: Duplicate entry points found.");
	    exit(1);
	}
	*entry_point_found = true;
    }

    auto frontend_end = std::chrono::high_resolution_clock::now();

    {
	std::lock_guard<std::mutex> lock(*metrics_mutex);

	// handle compiler flags
	if (flag_settings->print_ast) print_ast(ast);
	if (flag_settings->print_ir) print_ir(ir->_module);

	module_list->push_back(std::unique_ptr<llvm::Module>(ir->_module));
	metrics->total_lines += lexer->total_lines_postprocessing;

	// calculating the elapsed time duration in seconds
	std::chrono::duration<double> frontend_elapsed_time = frontend_end - frontend_start;
	metrics->frontend_time += frontend_elapsed_time.count();
    }

    // cleaning up allocated memory
    delete lexer;
    delete ast;
}


int main(int argc, char **argv)
{
    // keeping track of the execution time for benchmarking metrics
    auto frontend_start = std::chrono::high_resolution_clock::now();

    /*
    The basic compilation command should be something like:

	<compiler> <FILE_1> ... <FILE_n> ...
	                                  ^ flags (optional)
    */

    if (argc < 2) {
	fprintf(stderr, "ERROR: Provide the path of the file to be compiled.");
	exit(1);
    }

    int flags_start_index = 2;
    bool flags_exist = false;

    for (int i = 2; i < argc; i++) {
	if (argv[i][0] == '-') {
	    flags_start_index = i;
	    flags_exist = true;
	    break;
	}
    }

    Flag_Settings flag_settings;
    bool show_benchmarking_metrics = false;

    // set the compiler flag settings
    if (flags_exist) {
	for (int i = flags_start_index; i < argc; i++) {
	    if (strcmp(argv[i], "-pout") == 0) flag_settings.print_ast = true;
	    else if (strcmp(argv[i], "-llout") == 0) flag_settings.print_ir = true;
	    else if (strcmp(argv[i], "-ll") == 0) flag_settings.output_file_type = LL;
	    else if (strcmp(argv[i], "-asm") == 0) flag_settings.output_file_type = ASM;
	    else if (strcmp(argv[i], "-benchmark") == 0) show_benchmarking_metrics = true;
	    else if (strcmp(argv[i], "-cpu") == 0 && i < argc - 1) {
		flag_settings.cpu_type = argv[++i]; // reads the next argument as the cpu type
	    }
	    else if (strcmp(argv[i], "-o") == 0 && i < argc -1) {
		flag_settings.output_file_name = argv[++i]; // reads the next argument as output file name
	    }
	}
    }

    // run the compilation frontend for each file in parallel
    Compilation_Metrics metrics;
    bool entry_point_found = false;
    std::mutex metrics_mutex;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<llvm::Module>> module_list;

    int last_file_arg_index = flags_exist ? flags_start_index - 1 : argc - 1;

    for (int i = 1; i <= last_file_arg_index; i++) {
	threads.emplace_back([&, i]() {
	    compile(
	        argv[i],
	        &flag_settings,
		&entry_point_found,
	        frontend_start,
	        &metrics,
	        &metrics_mutex,
	        &module_list
	    );
	});
    }
    for (auto& t : threads) t.join(); // wait for all threads to finish

    // ensure that entry point exists
    if (!entry_point_found) {
	fprintf(stderr, "ERROR: No entry point (main) found.");
	exit(1);
    }

    // backend process begins
    auto backend_start = std::chrono::high_resolution_clock::now();

    // in order to link all the modules together
    // we must first bring them all under a single
    // shared context. to do this, we will have to
    // move/clone each module to the shared context.

    llvm::LLVMContext shared_context; // holds the global LLVM context
    std::vector<std::unique_ptr<llvm::Module>> unified_modules;

    unified_modules.reserve(module_list.size());

    // clone the module into the shared_context
    for (size_t i = 0; i < module_list.size(); ++i) {
	auto cloned_module = move_module_to_context(module_list[i].get(), shared_context);
	unified_modules.push_back(std::move(cloned_module));
	module_list[i] = nullptr;
    }

    // link the modules into a single module
    std::unique_ptr<llvm::Module> linked_module = link_modules(std::move(unified_modules));

    if (llvm::verifyModule(*linked_module, &llvm::errs())) {
	fprintf(stderr, "LINKER ERROR: Merged module verification failed.\n");
	exit(1);
    }

    // preparing for LLVM backend execution
    std::string target_triple;
    if (flag_settings.cpu_type != "") {
	for (int i = 0; i < NUM_CPU_TYPES; i++) {
	    if (cpu_to_target[i][0] == flag_settings.cpu_type) {
		target_triple = cpu_to_target[i][1];
		break;
	    }
	}
    }
    if (target_triple == "") flag_settings.cpu_type = "generic";

    std::string file_extension;
    switch (flag_settings.output_file_type) {
        case OBJ: file_extension = ".o"; break;
        case ASM: file_extension = ".s"; break;
        case LL: file_extension = ".ll"; break;
        default: fprintf(stderr, "ERROR: Invalid output file extension encountered."); exit(1);
    }
    std::string output_file_name = flag_settings.output_file_name + file_extension;

    // generate the output file for the particular target cpu
    if (flag_settings.output_file_type != LL) {
	run_llvm_backend(
            linked_module.get(),
            output_file_name,
            flag_settings.output_file_type,
            flag_settings.cpu_type,
            target_triple
	);
    } else write_llvm_ir_to_file(output_file_name.c_str(), linked_module.get());

    auto backend_end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> backend_elapsed_time = backend_end - backend_start;
    metrics.backend_time = backend_elapsed_time.count();

    if (show_benchmarking_metrics) {
	metrics.total_time = metrics.frontend_time + metrics.backend_time;
	print_benchmark_metrics(&metrics);
    }
    return 0;
}
