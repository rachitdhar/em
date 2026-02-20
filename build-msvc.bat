@echo off

set OPT_FLAG=/W0
set DEBUG_FLAG=
set CRT_FLAG=/MD

if "%1"=="Debug" (
   set OPT_FLAG=/Od
   set DEBUG_FLAG=/DEBUG
   set CRT_FLAG=/MDd
)

echo Build type: %1

cl ^
%OPT_FLAG% ^
%CRT_FLAG% ^
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
/LIBPATH:"D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\lib" ^
D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\lib\*.lib ^
%DEBUG_FLAG% ^
/OUT:emc.exe
