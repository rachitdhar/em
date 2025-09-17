parser: parser.cpp lexer.cpp lexer.h ast.h parser.h
	g++ parser.cpp lexer.cpp -o parser

clean:
	rm -f *.exe
