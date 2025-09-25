SRC = parser.cpp lexer.cpp codegen.cpp
CXX = g++
LLVM_INC = D:/softwares/llvm/include
LLVM_LIB = D:/softwares/llvm/lib
LLVM_LIBS = -lLLVMCore -lLLVMSupport
OUT = parser.exe

$(OUT): $(SRC)
	$(CXX) $(SRC) -I$(LLVM_INC) -L$(LLVM_LIB) $(LLVM_LIBS) -o $(OUT)

clean:
	rm -f *.exe
