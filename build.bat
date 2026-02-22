@echo off

if "%1"=="-debug" (
   set DEBUG_FLAG=-g -O0
) else (
   set DEBUG_FLAG=-w
)

clang++ ^
%DEBUG_FLAG% ^
lexer.cpp parser.cpp ir_generator.cpp main.cpp ^
-o ^
emc ^
-I ^
-std=c++17 ^
-fno-exceptions ^
-funwind-tables ^
-DEXPERIMENTAL_KEY_INSTRUCTIONS ^
-D_FILE_OFFSET_BITS=64 ^
-D__STDC_CONSTANT_MACROS ^
-D__STDC_FORMAT_MACROS ^
-D__STDC_LIMIT_MACROS ^
-L ^
D:/softwares/msys64/mingw64/lib ^
-lLLVM-21
