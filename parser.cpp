//
// parser.cpp
//

/*

Once we have the tokens, the goal is to be able to actually
structure them in a way so that we can embed the language
grammar (syntax), along with some level of semantic reasoning.

This is necessary so that later on, when generating instructions
we just have to translate logical units into the appropriate
instructions. Also, at this stage, much of the syntax error checking
will be handled. This will ensure that the user is actually
entering valid code.

What is this structure? We will use an AST (abstract syntax tree),
which is pretty standard for this purpose. But first, we must
specify the grammar rules for the language.


//                         Grammar
// *********************************************************

For our language, we will follow a C-style grammatical structure.
At the very top level, even before we reach statements, we have
two broad kinds of units:

    1. functions
    2. global definitions (for now we can just consider global variables)

These need to be handled first.

                      Top-level Program Unit
			      /    \
			Function   Global variable
			   |...

Now, further code only exists within the function blocks.
This is where things actually get crucial. Here, we have a concept
called a statement. Here, are the rules for what a statement can be
(in BNF / Backus-Naur Form):

    NOTE: <expr> : (expression) something that returns a value

    <statement> --> <expr> ;
                | if (<expr>) <statement> [ else <statement> ]
		| <loop-stmt>
		| <jump-stmt>
		| { <statement_1>, ..., <statement_n> }

    <loop-stmt> --> for (<expr>; <expr>; <expr>) <statement>
		  | while (<expr>) <statement>

    <jump-stmt> --> break;
                  | continue;
	          | return <expr>;

So when we are creating the AST, we need to ensure that these rules
are followed for the statements.

At the top level, a function in an AST would look like:

                     root
		     /  \...
	       function
	        /   \
	    params   body (<statement>)
	      |...         |...

Let's look at an example of a simple function body, and how
its function body AST segment would look.

Consider the function:

    f() {
	int x = 3;
	y++;
	return y;
    }

(Note that this is actually not a valid function. But it
is only semantically invalid. Syntactically it is fine.)

The AST segment for this should be something like:

                  function f
		   .../ \
		        <compound-stmt>
			 /     |     \
			=     ++    return
		       / \     |      |
		     int  3    y      y
		      |
		      x

This is the basic idea for this. However, we still have
to go over operator-precedence, which will be important when
evaluating many expressions.
*/

#include "ast.h"
#include "lexer.h"
#include "parser.h"


AST_Expression *parse_ast_expression(Lexer *lexer)
{
    // what are all possible kinds of expressions?
}


void parse_ast_block(vector<AST_Expression*> &block, Lexer *lexer)
{
    // the current token would be '{'
    // so we will get the next token

    /*
    The structure of a block is:
	{ <expr_1>; <expr_2>; ... <expr_n>; }
    */

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Missing \'}\' from scope.", lexer);
    }

    while (tok->type != TOKEN_RIGHT_BRACE) {
	// the current token is the beginning of some expression
	// OR
	// it is a ';' (delimiter)

	if (tok->type == TOKEN_DELIMITER) {
	    tok = lexer->get_next_token();
	    if (tok == NULL) {
		throw_parser_error("SYNTAX ERROR: Missing \'}\' from scope.", lexer);
	    }
	    continue;
	}

	AST_Expression *expr = parse_ast_expression(lexer);
	block.push_back(expr);
    }
}


// parses the function params, and adds them to ast_function->params
void parse_ast_function_params(AST_Function_Definition *ast_function, Lexer *lexer)
{
    // here we are assuming that the current token is '('
    // so we need to start reading from the next token

    /*
    Function parameter syntax:
	( <data_type1> <name_1>, <data_type2> <name_2>, ... )
    */

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_error__insufficient_tokens_func_def(lexer);
    }

    while (tok != NULL && tok->type != TOKEN_RIGHT_PAREN) {
	if (tok->type != TOKEN_DATA_TYPE) {
	    throw_parser_error("SYNTAX ERROR: Invalid data type for function parameter.", lexer);
	}
	auto *function_param = new Function_Parameter;
	function_param->type = tok->val;

	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_error__insufficient_tokens_func_def(lexer);
	}
	if (tok->type != TOKEN_IDENTIFIER) {
	    throw_parser_error("SYNTAX ERROR: Invalid identifier for function parameter.", lexer);
	}
	function_param->name = tok->val;

	// add this param to the list of parameters
	ast_function->params.push_back(function_param);

	// if next token is a comma, peek the token just after it
	// if it is not a comma, it must be a right parenthesis
	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_error__insufficient_tokens_func_def(lexer);
	}
	if (tok->type == TOKEN_SEPARATOR) tok = lexer->get_next_token();
	else if (tok->type == TOKEN_RIGHT_PAREN) break;
	else {
	    throw_parser_error("SYNTAX ERROR: Missing separator (\',\') in function parameters.", lexer);
	}
    }
    if (tok == NULL) throw_error__insufficient_tokens_func_def(lexer);
}


// parses the entire function (and everything within it)
AST_Function_Definition *parse_ast_function(Lexer *lexer)
{
    // Function syntax:
    // <return_type> <name>(...) <block>|<expr>

    // return type
    Token *tok_return_type = lexer->peek(0);
    if (tok_return_type->type != TOKEN_DATA_TYPE) {
	throw_parser_error("SYNTAX ERROR: Invalid return type for function definition.", lexer);
    }

    auto *ast_function = new AST_Function_Definition;
    ast_function->return_type = tok_return_type->val;

    // function name
    Token *tok_name = lexer->get_next_token();
    if (tok_name == NULL) {
	throw_error__insufficient_tokens_func_def(lexer);
    }
    if (tok_name->type != TOKEN_IDENTIFIER) {
	throw_parser_error("SYNTAX ERROR: Invalid identifier used in function definition.", lexer);
    }
    ast_function->function_name = tok_name->val;

    // left parenthesis
    Token *tok_left_paren = lexer->get_next_token();
    if (tok_left_paren == NULL) {
	throw_error__insufficient_tokens_func_def(lexer);
    }
    if (tok_left_paren->type != TOKEN_LEFT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing token \'(\' in function definition.", lexer);
    }

    // reading function parameters (if they exist)
    // after this is completed, the current token is ')'
    parse_ast_function_params(ast_function, lexer);

    // At the end of the function definition there are two possibilities
    //    1. We get a { --> this is a statement block
    //    2. We get a single statement

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Function definition must be followed by a statement.", lexer);
    }
    if (tok->type == TOKEN_LEFT_BRACE) parse_ast_block(ast_function->block, lexer);
    else {
	AST_Expression *expr = parse_ast_expression(lexer);
	ast_function->block.push_back(expr);
    }
    return ast_function;
}


void parse_tokens(Lexer *lexer)
{
    vector<Token> tokens = lexer->tokens;
    if (tokens.size() == 0) {
	throw_parser_error("ERROR: No tokens found.", lexer);
    }

    auto ast = new vector<AST_Expression*>; // abstract syntax tree initialized

    // TODO: Handle global variables

    while (lexer->curr_token_index < tokens.size()) {
	// at the outermost, we only have function definitions

	AST_Function_Definition *ast_function = parse_ast_function(lexer);
	ast.push_back(ast_function);
    }
}


int main()
{
    Lexer lexer;
    perform_lexical_analysis(&lexer, "program.txt");

    parse_tokens(&lexer);
    return 0;
}
