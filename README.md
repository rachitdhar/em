# Vanta (Programming Language)
Compiler for the Vanta programming language, a systems programming language with a C-style syntax that emphasizes clean, simple and minimal features with high performance.

This project was largely undertaken by me for fun and educational purposes, but it is semi-serious in the sense that it is a full-fledged compiler that can be used for real purposes.

> **Note:** This project is still in early development.

## Usage

Compile a file at a given path using:

```
vanta <FILE_PATH>
```

## Flags

We can add some compilation flags when compiling, as:

```
vanta <FILE_PATH> ...
                   ^ flags (optional)
```

Here are the flags that can be added, along with their purposes:

- **-pout** : Prints the Parser Output (structure of the AST)
- **-llout** : Prints the LLVM IR generated
- **-ll** : Generates a .ll file (LLVM IR) instead of an executable
- **-asm** : Generates a .s file (Assembly) instead of an executable
- **-benchmark** : Prints the performance metrics for the compilation process

## Compiling the Compiler + LLVM Linking

It turns out that one of the most difficult and irritating parts of developing this compiler was to figure out how LLVM libraries can actually be included and compiled. After doing almost every possible thing - from installing the LLVM binaries and manually linking; using Visual Studio configurations; using CMAKE to handle builds; to building LLVM myself from the llvm-project source and trying to fix the compatibility issues between it and my own mingw compiler - I finally found that MSYS2 comes with the ability to install everything I need: C/C++ compilers, GDB, LLC,... and most importantly all the LLVM headers and libraries. From within the MSYS MINGW64 shell, all the compilation problems get handled very smoothly.

At the location msys64/mingw64/bin, running the command:

```
llvm-config --cxxflags --ldflags --libs all --system-libs
```

gives me the output:

```
-ID:/softwares/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -DEXPERIMENTAL_KEY_INSTRUCTIONS -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -LD:/softwares/msys64/mingw64/lib -lLLVM-21
```

which contains the information needed regarding the include paths, compiler flags and library paths that are needed for linking LLVM during the compilation of my compiler.

I am using nob to write the C script to compile my compiler. It uses the nob.h single header library (from Tsoding, inspired by his "No-Build" concept).

Of course, this means before using nob, we must compile it too!

```
gcc nob.c -o nob
```

Then, to compile the compiler, just do:

```
./nob
```

To compile with debugging symbols, use the -debug flag:

```
./nob -debug
```
