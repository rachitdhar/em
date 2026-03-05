//
// type_checker.cpp
//

#include "parser.h"

void perform_type_checking(std::vector<AST_Expression *> *ast) {}

int main() {
    Lexer *lexer = perform_lexical_analysis("program.txt");
    auto *ast = parse_tokens(lexer);

    perform_type_checking(ast);
    return 0;
}
