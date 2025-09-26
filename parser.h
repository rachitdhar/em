//
// parser.h
//


#ifndef PARSER_H
#define PARSER_H

#include "ast.h"



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
inline Precedence op_prec(Token_Type type)
{
    switch (type)
    {
	// Assignment operators
    case TOKEN_ASSIGN:
    case TOKEN_PLUSEQ:
    case TOKEN_MINUSEQ:
    case TOKEN_MULTIPLYEQ:
    case TOKEN_DIVIDEEQ:
    case TOKEN_MODEQ:
    case TOKEN_OREQ:
    case TOKEN_BIT_OREQ:
    case TOKEN_XOREQ:
    case TOKEN_ANDEQ:
    case TOKEN_BIT_ANDEQ:
	return PREC_ASSIGNMENT;

	// OR operators
    case TOKEN_OR:
    case TOKEN_BIT_OR:
    case TOKEN_XOR:
	return PREC_OR;

	// AND operators
    case TOKEN_AND:
    case TOKEN_AMPERSAND:
	return PREC_AND;

	// Equality operators
    case TOKEN_EQUAL:
    case TOKEN_NOTEQ:
	return PREC_EQUALITY;

	// Comparison operators
    case TOKEN_LESS:
    case TOKEN_LESSEQ:
    case TOKEN_GREATER:
    case TOKEN_GREATEREQ:
	return PREC_COMPARISON;

	// Additive operators
    case TOKEN_PLUS:
    case TOKEN_MINUS:
	return PREC_ADDITIVE;

	// Multiplicative operators
    case TOKEN_STAR:
    case TOKEN_DIVIDE:
	return PREC_MULTIPLICATIVE;

	// Unary operators
    case TOKEN_NOT:
    case TOKEN_BIT_NOT:
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
	return PREC_UNARY;

	// Primary tokens
    case TOKEN_IDENTIFIER:
    case TOKEN_DATA_TYPE:
    case TOKEN_NUMERIC_LITERAL:
    case TOKEN_CHAR_LITERAL:
    case TOKEN_STRING_LITERAL:
    case TOKEN_LEFT_PAREN:
	return PREC_PRIMARY;

    default:
	return PREC_MIN;
    }
}


//                      Error printing
// ********************************************************

// to print 2-space indentations
inline void print_indentation(int indentation_level)
{
    for (int i = 0; i < 2 * indentation_level; i++) putchar(' ');
}


void print_ast_expression(AST_Expression *ast_expr, int indentation_level)
{
    // depending on the indentation level
    // we will add spaces before printing

    switch (ast_expr->expr_type) {
    case EXPR_IDENT: {
	print_indentation(indentation_level);
	printf("<IDENT, %s>\n", ((AST_Identifier*)ast_expr)->name.c_str());
	break;
    }
    case EXPR_LITERAL: {
	print_indentation(indentation_level);
	printf("<LITERAL>\n");
	break;
    }
    case EXPR_FUNC_DEF: {
	auto *expr = (AST_Function_Definition*)ast_expr;
	print_indentation(indentation_level);
	printf("<FUNC, %s> (", expr->function_name.c_str());

	for (Function_Parameter *param : expr->params) {
	    printf("[%s : Type %d]", param->name.c_str(), param->type);
	}
	printf(") -> (Type %d) {\n", expr->return_type);

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf("}\n");
	break;
    }
    case EXPR_IF: {
	auto *expr = (AST_If_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("<IF> (\n");
	print_ast_expression(expr->condition, indentation_level + 1);
	print_indentation(indentation_level);
	printf(") {\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf("}\n");

	if (expr->else_block.size() > 0) {
	    print_indentation(indentation_level);
	    printf("<ELSE> {\n");
	    for (AST_Expression *e : expr->else_block) {
		print_ast_expression(e, indentation_level + 1);
	    }
	    print_indentation(indentation_level);
	    printf("}\n");
	}
	break;
    }
    case EXPR_FOR: {
	auto *expr = (AST_For_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("<FOR> (\n");
	if (expr->init != NULL) print_ast_expression(expr->init, indentation_level + 1);
	if (expr->condition != NULL) print_ast_expression(expr->condition, indentation_level + 1);
	if (expr->increment != NULL) print_ast_expression(expr->increment, indentation_level + 1);
	print_indentation(indentation_level);
	printf(") {\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf("}\n");
	break;
    }
    case EXPR_WHILE: {
	auto *expr = (AST_While_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("<WHILE> (\n");
	print_ast_expression(expr->condition, indentation_level + 1);
	print_indentation(indentation_level);
	printf(") {\n");

	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf("}\n");
	break;
    }
    case EXPR_DECL: {
	auto *expr = (AST_Declaration*)ast_expr;
	print_indentation(indentation_level);
	printf("<DECL, [%s : Type %d]>\n", expr->variable_name.c_str(), expr->data_type);
	break;
    }
    case EXPR_UNARY: {
	auto *expr = (AST_Unary_Expression*)ast_expr;
	if (expr->op != TOKEN_NONE) {
	    print_indentation(indentation_level);
	    printf("<UNARY_OP (%s) : Type %d> (\n", (expr->is_postfix) ? "POST" : "PRE", (int)expr->op);
	    if (expr->expr != NULL) {
		print_ast_expression(expr->expr, indentation_level + 1);
	    }
	    print_indentation(indentation_level);
	    printf(")\n");
	    break;
	}
	if (expr->expr != NULL) {
	    print_ast_expression(expr->expr, indentation_level);
	}
	break;
    }
    case EXPR_BINARY: {
	auto *expr = (AST_Binary_Expression*)ast_expr;
	if (expr->op != TOKEN_NONE) {
	    print_indentation(indentation_level);
	    printf("<BINARY_OP : Type %d> (\n", (int)expr->op);
	    if (expr->left != NULL) {
		print_ast_expression(expr->left, indentation_level + 1);
	    }
	    if (expr->right != NULL) {
		print_ast_expression(expr->right, indentation_level + 1);
	    }
	    print_indentation(indentation_level);
	    printf(")\n");
	    break;
	}
	if (expr->left != NULL) {
	    print_ast_expression(expr->left, indentation_level);
	}
	if (expr->right != NULL) {
	    print_ast_expression(expr->right, indentation_level);
	}
	break;
    }
    case EXPR_FUNC_CALL: {
	auto *expr = (AST_Function_Call*)ast_expr;
	print_indentation(indentation_level);
	printf("<CALL, %s> (\n", expr->function_name.c_str());

	for (AST_Expression *e : expr->params) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf(")\n");
	break;
    }
    case EXPR_RETURN: {
	auto *expr = (AST_Return_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("<RETURN> (");
	if (expr->value != NULL) {
	    printf("\n");
	    print_ast_expression(expr->value, indentation_level + 1);
	    print_indentation(indentation_level);
	}
	printf(")\n");
	break;
    }
    case EXPR_JUMP: {
	auto *expr = (AST_Jump_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("<JUMP, %s>\n", expr->jump_type == J_BREAK ? "break" : "continue");
	break;
    }
    case EXPR_BLOCK: {
	auto *expr = (AST_Block_Expression*)ast_expr;
	print_indentation(indentation_level);
	printf("{\n");
	for (AST_Expression *e : expr->block) {
	    print_ast_expression(e, indentation_level + 1);
	}
	print_indentation(indentation_level);
	printf("}\n");
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
	printf("************** :: %d :: **************\n\n", top_expression_num);

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

inline void throw_error__used_delimiter_in_a_non_statement(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Invalid expression. Used \';\' in an expression that is not a statement.", lexer);
}


//                  Function definitions
// ***********************************************************

AST_Expression *parse_ast_subexpression(Lexer *lexer, Precedence curr_precedence, Token_Type stops_at = TOKEN_DELIMITER);
AST_Expression *parse_ast_expression(Lexer *lexer);
std::vector<AST_Expression*> *parse_tokens(Lexer *lexer);

#endif
