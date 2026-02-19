@echo off

if "%1"=="-debug" (
   set DEBUG_FLAG=/Od
) else (
   set DEBUG_FLAG=/W0
)

cl ^
%DEBUG_FLAG% ^
/std:c++17 ^
/DEXPERIMENTAL_KEY_INSTRUCTIONS ^
/D_FILE_OFFSET_BITS=64 ^
/D__STDC_CONSTANT_MACROS ^
/D__STDC_FORMAT_MACROS ^
/D__STDC_LIMIT_MACROS ^
/Zi ^
/I "D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\include" ^
lexer.cpp parser.cpp ir_generator.cpp main.cpp ^
/link ^
ntdll.lib ^
/LIBPATH:"D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\lib" ^
D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\lib\*.lib ^
/OUT:emc.exe
