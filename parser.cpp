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



void parse_ast_block(std::vector<AST_Expression*> &block, Lexer *lexer)
{
    // one possibility is that this is not a block
    // and just a single statement/expression

    if (lexer->peek(0)->type != TOKEN_LEFT_BRACE) {
	AST_Expression *expr = parse_ast_expression(lexer);
	block.push_back(expr);
	return;
    }

    // if it is a block:
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
	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_parser_error("SYNTAX ERROR: Missing \'}\' from scope.", lexer);
	}

	AST_Expression *expr = parse_ast_expression(lexer);
	block.push_back(expr);
    }
    tok = lexer->get_next_token();
}


inline AST_If_Expression *parse_ast_if_expression(Lexer *lexer)
{
    // currently the token must be an "if"
    // so we will start from the next token

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'if\' statement encountered.", lexer);
    }
    if (tok->type != TOKEN_LEFT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing \'(\' from if statement condition.", lexer);
    }
    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'if\' statement encountered.", lexer);
    }

    // parse the expression
    AST_Expression *condition = parse_ast_expression(lexer);

    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'if\' statement encountered.", lexer);
    }
    if (tok->type != TOKEN_RIGHT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing \')\' from if statement condition.", lexer);
    }
    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'if\' statement encountered.", lexer);
    }

    auto *ast_if = new AST_If_Expression;
    ast_if->condition = condition;

    // parse if block
    parse_ast_block(ast_if->block, lexer);

    // check whether there is an else block as well
    tok = lexer->peek(0);
    if (tok != NULL && tok->val == "else") {
	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_parser_error("SYNTAX ERROR: Incomplete \'else\' statement encountered.", lexer);
	}

	// parse else block
	parse_ast_block(ast_if->else_block, lexer);
    }
    return ast_if;
}


inline AST_For_Expression *parse_ast_for_expression(Lexer *lexer)
{
    // currently the token must be a "for"
    // so we will start from the next token

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }
    if (tok->type != TOKEN_LEFT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing \'(\' from for statement condition.", lexer);
    }
    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }

    // parse the initialization
    AST_Expression *init = NULL;
    if (tok->type != TOKEN_DELIMITER) {
	init = parse_ast_expression(lexer);

	tok = lexer->get_next_token();
	if (tok == NULL || tok != TOKEN_DELIMITER) {
	    throw_parser_error("SYNTAX ERROR: Missing \';\' after \'for\' expression initialization.", lexer);
	}
    }

    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }

    // parse the condition
    AST_Expression *condition = NULL;
    if (tok->type != TOKEN_DELIMITER) {
	condition = parse_ast_expression(lexer);

	tok = lexer->get_next_token();
	if (tok == NULL || tok != TOKEN_DELIMITER) {
	    throw_parser_error("SYNTAX ERROR: Missing \';\' after \'for\' expression condition.", lexer);
	}
    }

    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }

    // parse the increment
    AST_Expression *increment = NULL;
    if (tok->type != TOKEN_DELIMITER) {
	increment = parse_ast_expression(lexer);
    }

    tok = lexer->get_next_token();
    if (tok == NULL || tok->type != TOKEN_RIGHT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }

    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'for\' statement encountered.", lexer);
    }

    auto *ast_for = new AST_For_Expression;
    ast_for->init = init;
    ast_for->condition = condition;
    ast_for->increment = increment;

    // parse the block
    parse_ast_block(ast_for->block, lexer);

    return ast_for;
}


inline AST_While_Expression *parse_ast_while_expression(Lexer *lexer)
{
    // currently the token must be a "while"
    // so we will start from the next token

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'while\' statement encountered.", lexer);
    }
    if (tok->type != TOKEN_LEFT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing \'(\' from while statement condition.", lexer);
    }
    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'while\' statement encountered.", lexer);
    }

    // parse the expression
    AST_Expression *condition = parse_ast_expression(lexer);

    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'while\' statement encountered.", lexer);
    }
    if (tok->type != TOKEN_RIGHT_PAREN) {
	throw_parser_error("SYNTAX ERROR: Missing \')\' from if statement condition.", lexer);
    }
    tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'while\' statement encountered.", lexer);
    }

    auto *ast_while = new AST_While_Expression;
    ast_while->condition = condition;

    // parse while block
    parse_ast_block(ast_while->block, lexer);

    return ast_while;
}


inline AST_Return_Expression *parse_ast_return_expression(Lexer *lexer)
{
    // currently the token must be a "return"
    // so we will start from the next token

    Token *tok = lexer->get_next_token();
    if (tok == NULL) {
	throw_parser_error("SYNTAX ERROR: Incomplete \'return\' statement encountered.", lexer);
    }

    // there are two possibilities
    //     1. nothing is returned, and we have a ;
    //     2. there is some expression, and then a ;

    auto *ast_return = new AST_Return_Expression;

    if (tok->type == TOKEN_DELIMITER) {
	ast_return->value = NULL;
    } else {
	AST_Expression *expr = parse_ast_expression(lexer);
	ast_return->value = expr;

	tok = lexer->peek(0);
	if (tok == NULL || tok->type != TOKEN_DELIMITER) {
	    throw_error__missing_delimiter(lexer);
	}
    }
    return ast_return;
}


inline AST_Jump_Expression *parse_ast_jump_expression(Lexer *lexer, std::string jump_type)
{
    // currently the token must be either a "break" or "continue"
    // so we will start from the next token

    auto *ast_jump = new AST_Jump_Expression;
    ast_jump->jump_type = jump_type;

    Token *tok = lexer->get_next_token();
    if (tok == NULL || tok->type != TOKEN_DELIMITER) {
	throw_error__missing_delimiter(lexer);
    }
    return ast_jump;
}


inline AST_Function_Call *parse_ast_function_call(Lexer *lexer)
{
    // currently the token must be at the function name (i.e., an identifier)
    // we will read that token, and then move ahead.

    auto ast_call = new AST_Function_Call;

    Token *tok = lexer->peek(0);
    ast_call->function_name = tok->val;

    /*
    The argument structure is:

	( <arg_1>, <arg_2>, ..., <arg_n> )

    where, each <arg_k>, can be some kind of expression.
    */

    tok = lexer->get_next_token(); // this is '('
    // we won't check if it is NULL
    // because we already checked this, and only due to that
    // was the parse_ast_function_call() called in the first place.

    tok = lexer->get_next_token();

    while (tok->type != TOKEN_RIGHT_PAREN) {
	AST_Expression *arg_expr = parse_ast_expression(lexer);
	ast_call->params.push_back(arg_expr);

	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_parser_error("SYNTAX ERROR: Incomplete function call expression.", lexer);
	}
	if (tok->type == TOKEN_SEPARATOR) {
	    tok = lexer->get_next_token();
	    if (tok == NULL) {
		throw_parser_error("SYNTAX ERROR: Incomplete function call expression.", lexer);
	    }
	}
    }
    return ast_call;
}


/*

TODO:

    1. Need to handle pointers everywhere where identifiers exist.
    as of now i've just ignored their existence.

    2. Need to handle arrays (through []), again, where identifiers exist.
*/

// this is the core of the parser
// basically the main job of a parser is to be able to parse expressions
// this is also the most "complicated" part, so to speak. the main thing
// that makes it complicated is handling operations. here I will be trying
// to implement a recursive descent kind of parser.
AST_Expression *parse_ast_expression(Lexer *lexer)
{
    // what are all possible kinds of expressions?
    // well there are a lot. so we will need to find
    // a way to look through the tokens and figure out
    // the kind of expression.

    Token *tok = lexer->peek(0);

    //     POSSIBILITY 1 : starts with a keyword

    //     if ( <expression> ) <block>
    //     for ( <expression> ; <expression> ; <expression> ) <block>
    //     while ( <expression> ) <block>
    //     return <expression>;
    //     break; OR continue;

    if (tok->type == TOKEN_KEYWORD) {
	if (tok->val == "if") return parse_ast_if_expression(lexer);
	else if (tok->val == "for") return parse_ast_for_expression(lexer);
	else if (tok->val == "while") return parse_ast_while_expression(lexer);
	else if (tok->val == "return") return parse_ast_return_expression(lexer);
	else if (tok->val == "break") return parse_ast_jump_expression(lexer, "break");
	else if (tok->val == "continue") return parse_ast_jump_expression(lexer, "continue");
	else throw_parser_error("SYNTAX ERROR: Keyword could not be parsed.", lexer);
    }

    // POSSIBILITY 2 : does not start with a keyword

    /*
    Here's the thing. When trying to parse an expression,
    we could encounter the possibility that there is an operator
    ahead that needs to be considered.

    For instance:

	int x = 3 * func(5) - y - ++y;

    We need the AST tree for this part of the expression
    to look something like:

	           (=)
		   / \
	      int x   (-)
	              / \
		    (-)  (PRE ++)
		    / \     |
	          (*)  y    y
		  / \
		 3   func(5)

    To do this, we will read through tokens, till we reach the
    semicolon (;) at the end. Then, we will take all the operators
    that come up in the process (here: =, -, -, ++, and *) and
    form the tree for the expression as per the appropriate
    precedence. The operators of higher precedence are "deeper"
    in the tree (so that they get evaluated first).
    */

    // <literal>

    if (is_literal(tok)) {
	auto *ast_literal = new AST_Literal;

	if (tok->type == TOKEN_NUMERIC_LITERAL) {
	    // it is either an int or a float
	    // if it has a decimal point ('.'), then it is a float

	    if (tok->val.find('.') != std::string::npos) {
		ast_literal->value.f = std::stof(tok->val);
	    } else {
		ast_literal->value.i = std::stoi(tok->val);
	    }
	} else if (tok->type == TOKEN_CHAR_LITERAL) {
	    ast_literal->value.c = tok->val[0];
	} else {
	    ast_literal->value.s = tok->val;
	}
	return ast_literal;
    }

    // declaration
    //     <data_type> <identifier>

    if (tok->type == TOKEN_DATA_TYPE) {
	auto *ast_decl = new AST_Declaration;
	ast_decl->data_type = tok->val;

	tok = lexer->get_next_token();
	if (tok == NULL || tok->type != TOKEN_IDENTIFIER) {
	    throw_parser_error("SYNTAX ERROR: Invalid declaration. Missing identifier after data type", lexer);
	}

	ast_decl->variable_name = tok->val;
	return ast_decl;
    }

    // parenthesized expression
    //     ( <expression> )

    if (tok->type == TOKEN_LEFT_PAREN) {
	tok = lexer->get_next_token();
	if (tok == NULL) {
	    throw_parser_error("SYNTAX ERROR: Missing expression after \'(\'.", lexer);
	}

	AST_Expression *expr = parse_ast_expression(lexer);

	tok = lexer->get_next_token();
	if (tok == NULL || tok->type != TOKEN_RIGHT_PAREN) {
	    throw_parser_error("SYNTAX ERROR: Missing \')\' after expression.", lexer);
	}

	return expr;
    }

    // function call: <identifier> ( <args> ... )
    //
    // OR
    //
    // just a variable: <identifier>

    if (tok->type == TOKEN_IDENTIFIER) {
	// we need to "peek" ahead just to check in case
	// it might be actually a function call.
	// if we find a '(', we will assume that the
	// identifier is a function name

	Token *next = lexer->peek_next_token();
	if (next == NULL) {
	    throw_error__missing_delimiter(lexer);
	}
	if (next == TOKEN_LEFT_PAREN) {
	    // this is a function call
	    return parse_ast_function_call(lexer);
	}

	// if it is not a function call
	auto *ast_ident = new AST_Identifier;
	ast_ident->name = tok->val;

	return ast_ident;
    }

    // unary operation
    //     <expression> <postfix_unary_op> OR <prefix_unary_op> <expression>

    // binary operation
    //     <expresssion> <binary_op> <expression>

    return NULL; // in case nothing is parsed
}


// TODO: params could be pointers (to handle *)

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
    parse_ast_block(ast_function->block, lexer);
    return ast_function;
}


void parse_tokens(Lexer *lexer)
{
    if (lexer->tokens.size() == 0) {
	throw_parser_error("ERROR: No tokens found.", lexer);
    }

    auto ast = new std::vector<AST_Expression*>; // abstract syntax tree initialized

    // TODO: Handle global variables

    while (1) {
	if (lexer->peek(0) == NULL) break;

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
