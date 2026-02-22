# Em (Programming Language)
Compiler for the Em programming language, a systems programming language with a C-style syntax that emphasizes clean, simple and minimal features with high performance.

This project was largely undertaken by me for fun and educational purposes, but it is semi-serious in the sense that it is a full-fledged compiler that can be used for real purposes.

> **Note:** This project is still in early development.

## Usage

Compile a file at a given path using emc (the "Em compiler"):

```
emc <FILE_PATH>
```

The file must have a .em extension.
To compile multiple files at once:

```
emc <FILE_1> <FILE_2> ... <FILE_n>
```

## Flags

We can add some compilation flags when compiling, as:

```
emc <FILE_1> <FILE_2> ... <FILE_n> ...
                                    ^ flags (optional)
```

A flag is identified as something that starts with a hyphen (-).
Everything before the first flag will be treated as a file to be compiled.

Here are the flags that can be added, along with their purposes:

- **-pout** : Prints the Parser Output (structure of the AST)
- **-llout** : Prints the LLVM IR generated
- **-ll** : Generates a .ll file (LLVM IR) instead of an executable
- **-asm** : Generates a .s file (Assembly) instead of an executable
- **-cpu** : To specify the target CPU type. This flag must be followed by the CPU name
- **-o** : To name the output file (for any type). This flag must be followed by the file name
- **-benchmark** : Prints the performance metrics for the compilation process

The list of CPU types that can be set as targets using "-cpu", are:

```
x86-64
cortex-m3
cortex-m4
cortex-m7
cortex-a7
cortex-a53
cortex-a72
cortex-a76
cortex-a78
cortex-x1
apple-m1
apple-m2
neoverse-n1
neoverse-v1
neoverse-n2
```

If the "-cpu" flag is not specified, the cpu type of the host machine is determined by the compiler automatically at compile time.

For example, in order to compile a program called "prog.em" and get the assembly for the x86-64 target, we will compile using the command:

```
emc prog.em -asm -cpu x86-64
```

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

I have a build script to compile the compiler:

```
./build.bat
```

To compile with debugging symbols, use the -debug flag:

```
./build.bat -debug
```

## Compiling the Compiler (Visual Studio / MSVC cl.exe compiler + LLVM for Windows)

So here's the thing. Visual Studio uses the MSVC cl.exe compiler, which CAN actually be used to build this project.
The thing where I was going wrong was, I was trying to use the LLVM version for GNU based C++ compilers. What I did not
realise at the time was that this is a problem because the GNU based LLVM libraries and MSVC cl.exe compiler are not ABI
compatible, since of course, GNU compilers (used to build the GNU based LLVM) are just different from MSVC ones. So I had to use
the correct build of LLVM. I DID in fact try to do this earlier, and HAD even installed the one from the LLVM releases page. The
problem there was that it did not have all the libraries and libs I needed to be able to use it.

Turns out, that on their releases page itself, they have mentioned clearly that for Windows, the clang+llvm artifact is the one
that contains all the libraries and libs, which for some reason is this way only for Windows (otherwise for other cases, as I previously
assumed, it shouldn't matter which one you installed, as they are all the same LLVM version and under the same release).

Anyways, long story short, now I can actually build using the Visual Studio default compiler. Of course, I wrote a separate build script
for this purpose, in build-msvc.bat, which I configured Visual Studio to use as part of its Makefile project configuration. It took a
few hours figuring out how I am supposed to add the flags for linking and stuff, but now it seems to be working fine.