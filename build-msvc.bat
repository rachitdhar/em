@echo off

set DEBUG_FLAG=/W0
set DEBUG_LINK=

if "%1"=="Debug" (
   set DEBUG_FLAG=/Od
   set DEBUG_LINK=/DEBUG
)

echo Build type: %1

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
%DEBUG_LINK% ^
/OUT:emc.exe
