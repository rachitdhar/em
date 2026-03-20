//
// emc.h
//

#pragma once

#include "ir_generator.h"
#include <chrono>
#include <mutex>
#include <thread>

#define LANGUAGE_FILE_EXTENSION "em"


enum Output_File_Type { OBJ, ASM, LL };

struct Flag_Settings {
    bool print_ast = false;
    bool print_ir = false;
    Output_File_Type output_file_type = OBJ;
    std::string cpu_type;
    std::string output_file_name = "out";
    int optimization_level = 0;
};

struct Compilation_Metrics {
    size_t total_lines = 0;
    size_t num_threads = 1;
    double aggregate_frontend_time = 0;  // sum of frontend times of each thread
    double frontend_time = 0;      // total fronend time taken
    double backend_time = 0;       // total backend time taken (mainly by LLVM)
    double linking_time = 0;       // total linking time taken (for obj to exe creation)
    double total_time = 0;         // total time for entire compilation process
};

const std::string cpu_to_target[][2] = {
    /* Windows/Linux x86 systems */
    {"x86-64", "x86_64-unknown-linux-gnu"},

    /* Embedded / microcontrollers (ARM 32-bit) */
    {"cortex-m3", "armv7m-none-eabi"},
    {"cortex-m4", "armv7em-none-eabi"},
    {"cortex-m7", "armv7em-none-eabi"},

    /* Raspberry Pi / ARM 64-bit */
    {"cortex-a7", "armv7a-unknown-linux-gnueabihf"}, // Pi 2
    {"cortex-a53", "aarch64-unknown-linux-gnu"},     // Pi 3
    {"cortex-a72", "aarch64-unknown-linux-gnu"},     // Pi 4

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
    {"neoverse-n2", "aarch64-unknown-linux-gnu"}};

 const int NUM_CPU_TYPES = sizeof(cpu_to_target) / sizeof(cpu_to_target[0]);
