@echo off

if "%1"=="-debug" (
   set DEBUG_FLAG=-g -O0
) else (
   set DEBUG_FLAG=-w
)

clang++ ^
%DEBUG_FLAG% ^
src/lexer.cpp src/parser.cpp src/ir_generator.cpp src/dsa.cpp src/linker.cpp src/main.cpp ^
-o ^
bin/emc ^
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
