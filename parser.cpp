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

What is this structure? We will use an AST (abstract synatx tree),
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


void parse_tokens(Lexer *lexer)
{
    vector<Token> tokens = lexer->tokens;

    while (lexer->curr_token_index < tokens.size()) {

    }
}


int main()
{
    Lexer lexer;
    perform_lexical_analysis(&lexer, "program.txt");

    parse_tokens(&lexer);
    return 0;
}
