SRC = lexer.cpp parser.cpp ir_generator.cpp
CXX = g++
LLVM_INC = D:/softwares/llvm/include
LLVM_LIB = D:/softwares/llvm/lib
LLVM_LIBS = -lLLVMCore -lLLVMSupport
OUT = compiler.exe

$(OUT): $(SRC)
	$(CXX) $(SRC) -I$(LLVM_INC) -L$(LLVM_LIB) $(LLVM_LIBS) -o $(OUT)

clean:
	rm -f *.exe
