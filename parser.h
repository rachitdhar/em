//
// parser.h
//


#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <unordered_map>


// defining precedence levels
// (from lowest to highest)
enum Precedence {
    PREC_MIN, // minimum precedence (for default at the start of subexpression parsing)
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_ADDITIVE,
    PREC_MULTIPLICATIVE,
    PREC_UNARY,
    PREC_PRIMARY  // for identifiers, function calls, literals and parenthesized expressions
};


// map the operators to their precedence
std::unordered_map<Token_Type, Precedence> op_prec = {
    {TOKEN_ASSIGN,          PREC_ASSIGNMENT},
    {TOKEN_PLUSEQ,          PREC_ASSIGNMENT},
    {TOKEN_MINUSEQ,         PREC_ASSIGNMENT},
    {TOKEN_MULTIPLYEQ,      PREC_ASSIGNMENT},
    {TOKEN_DIVIDEEQ,        PREC_ASSIGNMENT},
    {TOKEN_MODEQ,           PREC_ASSIGNMENT},
    {TOKEN_OREQ,            PREC_ASSIGNMENT},
    {TOKEN_BIT_OREQ,        PREC_ASSIGNMENT},
    {TOKEN_XOREQ,           PREC_ASSIGNMENT},
    {TOKEN_ANDEQ,           PREC_ASSIGNMENT},
    {TOKEN_BIT_ANDEQ,       PREC_ASSIGNMENT},
    {TOKEN_OR,              PREC_OR},
    {TOKEN_BIT_OR,          PREC_OR},
    {TOKEN_XOR,             PREC_OR},
    {TOKEN_AND,             PREC_AND},
    {TOKEN_AMPERSAND,       PREC_AND},
    {TOKEN_EQUAL,           PREC_EQUALITY},
    {TOKEN_NOTEQ,           PREC_EQUALITY},
    {TOKEN_LESS,            PREC_COMPARISON},
    {TOKEN_LESSEQ,          PREC_COMPARISON},
    {TOKEN_GREATER,         PREC_COMPARISON},
    {TOKEN_GREATEREQ,       PREC_COMPARISON},
    {TOKEN_PLUS,            PREC_ADDITIVE},
    {TOKEN_MINUS,           PREC_ADDITIVE},
    {TOKEN_STAR,            PREC_MULTIPLICATIVE},
    {TOKEN_DIVIDE,          PREC_MULTIPLICATIVE},
    {TOKEN_NOT,             PREC_UNARY},
    {TOKEN_BIT_NOT,         PREC_UNARY},
    {TOKEN_INCREMENT,       PREC_UNARY},
    {TOKEN_DECREMENT,       PREC_UNARY},
    {TOKEN_IDENTIFIER,      PREC_PRIMARY},
    {TOKEN_DATA_TYPE,       PREC_PRIMARY},
    {TOKEN_NUMERIC_LITERAL, PREC_PRIMARY},
    {TOKEN_CHAR_LITERAL,    PREC_PRIMARY},
    {TOKEN_STRING_LITERAL,  PREC_PRIMARY},
    {TOKEN_LEFT_PAREN,      PREC_PRIMARY}
};


// to print 2-space indentations
inline void print_indentation(int indentation_level)
{
    for (int i = 0; i < 2 * indentation_level; i++) putchar(' ');
}


void print_ast_expression(AST_Expression *ast_expr, int indentation_level)
{
    // depending on the indentation level
    // we will add spaces before printing
    print_indentation(indentation_level);

    switch (ast_expr->expr_type) {
    case EXPR_IDENT: {
	printf("<IDENT, %s>\n", ((AST_Identifier*)ast_expr)->name.c_str());
	break;
    }
    case EXPR_LITERAL: {
	printf("<LITERAL>\n");
	break;
    }
    case EXPR_FUNC_DEF: {
	auto expr = (AST_Function_Definition*)ast_expr;
	printf("<FUNC, %s> (", expr->function_name.c_str());

	for (Function_Parameter *param : expr->params) {
	    printf("[%s %s]", param->type.c_str(), param->name.c_str());
	}
	printf(") -> (%s)\n", expr->return_type.c_str());

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	break;
    }
    case EXPR_IF: {
	auto expr = (AST_If_Expression*)ast_expr;
	printf("<IF> (\n");
	print_ast_expression(expr->condition, indentation_level + 1);
	print_indentation(indentation_level);
	printf(")\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}

	if (expr->else_block.size() > 0) {
	    print_indentation(indentation_level);
	    printf("<ELSE>\n");
	    for (AST_Expression *e : expr->else_block) {
		print_ast_expression(e, indentation_level + 1);
	    }
	}
	break;
    }
    case EXPR_FOR: {
	auto expr = (AST_For_Expression*)ast_expr;
	printf("<FOR> (\n");
	print_ast_expression(expr->init, indentation_level + 1);
	print_ast_expression(expr->condition, indentation_level + 1);
	print_ast_expression(expr->increment, indentation_level + 1);
	print_indentation(indentation_level);
	printf(")\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	break;
    }
    case EXPR_WHILE: {
	auto expr = (AST_While_Expression*)ast_expr;
	printf("<WHILE> (\n");
	print_ast_expression(expr->condition, indentation_level + 1);
	print_indentation(indentation_level);
	printf(")\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	break;
    }
    case EXPR_DECL: {
	auto expr = (AST_Declaration*)ast_expr;
	printf("<DECL, [%s %s]>\n", expr->data_type.c_str(), expr->variable_name.c_str());
	break;
    }
    case EXPR_BINARY: {
	auto expr = (AST_Binary_Expression*)ast_expr;
	if (expr->left != NULL) {
	    print_ast_expression(expr->left, indentation_level);
	}
	if (expr->op != TOKEN_NONE) {
	    print_indentation(indentation_level + 1);
	    printf("<OP, Type : %d>\n", (int)expr->op);
	}
	if (expr->right != NULL) {
	    print_ast_expression(expr->right, indentation_level);
	}
	break;
    }
    case EXPR_FUNC_CALL: {
	auto expr = (AST_Function_Call*)ast_expr;
	printf("<CALL, %s> (\n", expr->function_name.c_str());

	for (AST_Expression *e : expr->params) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf(")\n");
	break;
    }
    case EXPR_RETURN: {
	auto expr = (AST_Return_Expression*)ast_expr;
	printf("<RETURN> (\n");
	print_ast_expression(expr->value, indentation_level + 1);
	print_indentation(indentation_level);
	printf(")\n");
	break;
    }
    case EXPR_JUMP: {
	printf("<JUMP, %s>\n", ((AST_Jump_Expression*)ast_expr)->jump_type.c_str());
	break;
    }
    default: {
	fprintf(stderr, "ERROR: Failed to print AST expression.");
	exit(1);
    }
    }
}


// For Debugging only
// (to print the abstract syntax tree)
inline void print_ast(std::vector<AST_Expression*> *ast)
{
    int top_expression_num = 1;
    for (AST_Expression *ast_expr : *ast) {
	// printing an expression tree
	printf("AST Top Expression :: %d\n", top_expression_num);
	printf("***************************\n\n");

	print_ast_expression(ast_expr, 0);
	printf("\n");
	top_expression_num++;
    }
}


// get the line from the program file, using the line number
inline std::string get_file_line(std::string file_name, int line_num)
{
    // NOTE: this is not the most optimal
    // looking solution. but since it is only
    // being used to display error messages,
    // I am letting it be like this for now.

    std::ifstream file(file_name.c_str());
    std::string line;

    for (int i = 0; i < line_num; ++i) std::getline(file, line);
    std::getline(file, line);

    return line;
}


// get leading whitespace from a string (line)
inline std::string get_leading_whitespace(std::string line)
{
    size_t pos = line.find_first_not_of(" \t\n\r\f\v");

    std::string leading_ws = (pos != std::string::npos) ? line.substr(0, pos) : line;
    return leading_ws;
}


// to provide an error message, with error source information
// and then terminate the program execution.
void throw_parser_error(const char *message, Lexer *lexer)
{
    fprintf(stderr, message);

    // check if there exists any token at curr_token_index
    // if not, throw a fatal error

    Token *tok = lexer->peek(0);
    if (tok == NULL) {
	fprintf(stderr, "\nFATAL (Parser): Could not find current token.");
	exit(1);
    }

    printf(" [line %d, position %d]\n\n", tok->line_num, tok->position);

    // display the line where error occurred
    std::string line = get_file_line(lexer->file_name, tok->line_num - 1);
    std::string leading_ws = get_leading_whitespace(line);
    int error_pointer_pos = tok->position + 1 - leading_ws.size();

    printf("\t%s\n", line.c_str());
    printf("\t%s%*c\n", leading_ws.c_str(), error_pointer_pos, '^'); // to mark (using ^) the position of the error

    exit(1);
}


// insufficient token errors
inline void throw_error__insufficient_tokens_func_def(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Insufficient tokens for function definition.", lexer);
}


// missing delimiter ';' at end of statement
inline void throw_error__missing_delimiter(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Missing delimiter \';\' at the end of the statement.", lexer);
}


inline void throw_error__incomplete_func_call(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Incomplete function call expression.", lexer);
}


//                  Function definitions
// ***********************************************************

AST_Expression *parse_ast_subexpression(Lexer *lexer, Precedence curr_precedence, Token_Type stops_at = TOKEN_DELIMITER);
AST_Expression *parse_ast_expression(Lexer *lexer);
std::vector<AST_Expression*> *parse_tokens(Lexer *lexer);

#endif
