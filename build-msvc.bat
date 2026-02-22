@echo off

set OPT_FLAG=/O2
set DEBUG_FLAG=

if "%1"=="Clean" (
	del /Q *.exe *.pdb *.obj *.ilk *.idb
	echo Cleaned build artifacts.
	exit /B 0
)

if "%1"=="Debug" (
   set OPT_FLAG=/Od
   set DEBUG_FLAG=/Zi /DEBUG
)

echo Build type: %1

clang-cl ^
%OPT_FLAG% ^
/std:c++17 ^
/DEXPERIMENTAL_KEY_INSTRUCTIONS ^
/D_FILE_OFFSET_BITS=64 ^
/D__STDC_CONSTANT_MACROS ^
/D__STDC_FORMAT_MACROS ^
/D__STDC_LIMIT_MACROS ^
%DEBUG_FLAG% ^
/I "D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\include" ^
lexer.cpp parser.cpp ir_generator.cpp main.cpp ^
/link ^
ntdll.lib ^
/LIBPATH:"D:\softwares\clang+llvm-21.1.8-x86_64-pc-windows-msvc\lib" ^
LLVMWindowsManifest.lib ^
LLVMXRay.lib ^
LLVMLibDriver.lib ^
LLVMDlltoolDriver.lib ^
LLVMTelemetry.lib ^
LLVMTextAPIBinaryReader.lib ^
LLVMCoverage.lib ^
LLVMLineEditor.lib ^
LLVMNVPTXCodeGen.lib ^
LLVMNVPTXDesc.lib ^
LLVMNVPTXInfo.lib ^
LLVMRISCVTargetMCA.lib ^
LLVMRISCVDisassembler.lib ^
LLVMRISCVAsmParser.lib ^
LLVMRISCVCodeGen.lib ^
LLVMRISCVDesc.lib ^
LLVMRISCVInfo.lib ^
LLVMWebAssemblyDisassembler.lib ^
LLVMWebAssemblyAsmParser.lib ^
LLVMWebAssemblyCodeGen.lib ^
LLVMWebAssemblyUtils.lib ^
LLVMWebAssemblyDesc.lib ^
LLVMWebAssemblyInfo.lib ^
LLVMBPFDisassembler.lib ^
LLVMBPFAsmParser.lib ^
LLVMBPFCodeGen.lib ^
LLVMBPFDesc.lib ^
LLVMBPFInfo.lib ^
LLVMX86TargetMCA.lib ^
LLVMX86Disassembler.lib ^
LLVMX86AsmParser.lib ^
LLVMX86CodeGen.lib ^
LLVMX86Desc.lib ^
LLVMX86Info.lib ^
LLVMARMDisassembler.lib ^
LLVMARMAsmParser.lib ^
LLVMARMCodeGen.lib ^
LLVMARMDesc.lib ^
LLVMARMUtils.lib ^
LLVMARMInfo.lib ^
LLVMAArch64Disassembler.lib ^
LLVMAArch64AsmParser.lib ^
LLVMAArch64CodeGen.lib ^
LLVMAArch64Desc.lib ^
LLVMAArch64Utils.lib ^
LLVMAArch64Info.lib ^
LLVMOrcDebugging.lib ^
LLVMOrcJIT.lib ^
LLVMWindowsDriver.lib ^
LLVMMCJIT.lib ^
LLVMJITLink.lib ^
LLVMInterpreter.lib ^
LLVMExecutionEngine.lib ^
LLVMRuntimeDyld.lib ^
LLVMOrcTargetProcess.lib ^
LLVMOrcShared.lib ^
LLVMDWP.lib ^
LLVMDWARFCFIChecker.lib ^
LLVMDebugInfoLogicalView.lib ^
LLVMOption.lib ^
LLVMObjCopy.lib ^
LLVMMCA.lib ^
LLVMMCDisassembler.lib ^
LLVMLTO.lib ^
LLVMPasses.lib ^
LLVMHipStdPar.lib ^
LLVMCFGuard.lib ^
LLVMCoroutines.lib ^
LLVMipo.lib ^
LLVMVectorize.lib ^
LLVMSandboxIR.lib ^
LLVMLinker.lib ^
LLVMFrontendOpenMP.lib ^
LLVMFrontendOffloading.lib ^
LLVMObjectYAML.lib ^
LLVMFrontendOpenACC.lib ^
LLVMFrontendHLSL.lib ^
LLVMFrontendDriver.lib ^
LLVMInstrumentation.lib ^
LLVMFrontendDirective.lib ^
LLVMFrontendAtomic.lib ^
LLVMExtensions.lib ^
LLVMDWARFLinkerParallel.lib ^
LLVMDWARFLinkerClassic.lib ^
LLVMDWARFLinker.lib ^
LLVMGlobalISel.lib ^
LLVMMIRParser.lib ^
LLVMAsmPrinter.lib ^
LLVMSelectionDAG.lib ^
LLVMCodeGen.lib ^
LLVMTarget.lib ^
LLVMObjCARCOpts.lib ^
LLVMCodeGenTypes.lib ^
LLVMCGData.lib ^
LLVMIRPrinter.lib ^
LLVMInterfaceStub.lib ^
LLVMFileCheck.lib ^
LLVMFuzzMutate.lib ^
LLVMScalarOpts.lib ^
LLVMInstCombine.lib ^
LLVMAggressiveInstCombine.lib ^
LLVMTransformUtils.lib ^
LLVMBitWriter.lib ^
LLVMAnalysis.lib ^
LLVMProfileData.lib ^
LLVMSymbolize.lib ^
LLVMDebugInfoBTF.lib ^
LLVMDebugInfoPDB.lib ^
LLVMDebugInfoMSF.lib ^
LLVMDebugInfoCodeView.lib ^
LLVMDebugInfoGSYM.lib ^
LLVMDebugInfoDWARF.lib ^
LLVMDebugInfoDWARFLowLevel.lib ^
LLVMObject.lib ^
LLVMTextAPI.lib ^
LLVMMCParser.lib ^
LLVMIRReader.lib ^
LLVMAsmParser.lib ^
LLVMMC.lib ^
LLVMBitReader.lib ^
LLVMFuzzerCLI.lib ^
LLVMCore.lib ^
LLVMRemarks.lib ^
LLVMBitstreamReader.lib ^
LLVMBinaryFormat.lib ^
LLVMTargetParser.lib ^
LLVMTableGen.lib ^
LLVMSupport.lib ^
LLVMDemangle.lib ^
/OUT:emc.exe
