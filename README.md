# compiler
A compiler (in development)

## LLVM Linking

At the location llvm-build/bin/, running the command:

```
llvm-config --cxxflags --ldflags --libs core support --system-libs
```

gives me the output:

```
-ID:/softwares/mingw64/include -std=c++17 -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -LD:/softwares/mingw64/lib -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMBinaryFormat -lLLVMTargetParser -lLLVMSupport -lLLVMDemangle
```

which contains the information needed regarding the compiler flags and library flags that are needed for linking LLVM during the compilation of my compiler.
