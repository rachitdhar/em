// Lexer.cpp

#include "lexer.h"

/*

While reading characters in a line, we need to consider various scenarios.

Let's say we know we are now reading something that is not a whitespace.
Call it char (c)

Now, (c) could be some:

    1. alphabet / underscore
    2. number
    3. special symbol

In each case, we need to also look at the previously stored string (curr).
This is because some tokens are multiple chars long, but it is also
possible that someone wrote two tokens without any spaces between them.

For ex: x==30

Here, we have:
    <x, IDENTIFIER>
    <==, EQUAL>
    <30, NUMERIC_LITERAL>

So it seems that we need to do 3 things:

    check if there was something in curr earlier
    if yes, is (c) a continuation of it, or a part of a new token?

    if it is a continuation, just add (c) to curr, and continue
    otherwise:
	handle the curr, and create a token
	clear curr
	add (c) to curr, and continue


Here are a few ways things can be simplified a bit:

    1. when a bracket is encountered, don't add it to curr,
    instead, directly make it a token. same goes for char literals
    and string literals (just go through them until the closing quote)

    2. when anything is to be added to curr, also store a
    "partial type", like, "NUMERIC", "ALPHA_NUMERIC", ...

    3. when encountering a symbol, it is best to just go through
    with it, and read the next 2-3 chars (checking whether you have
    identified the token at each point or not) there itself instead
    of storing it in curr.

So basically the only thing that could ever be in curr
is numeric / alphanumeric type content, which can
only map to an IDENTIFIER, KEYWORD, or NUMERIC_LITERAL

******* KEY INSIGHT *******

This is great, because now it is clear that, if we are dealing with:

    1. brackets OR
    2. symbols OR
    3. quotes (' or ")

AND, if curr is not empty, then curr MUST be a different token,
and this is NOT a continuation.
*/


void make_token_as_per_ptok(Lexer* lexer, std::string& curr, Partial_Token_Type ptok)
{
    if (ptok == PTOK_NUMERIC) {
	lexer->tokens.push_back(new Token{curr, TOKEN_NUMERIC_LITERAL});
	return;
    }

    size_t len = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
    for (int i = 0; i < len; i++) {
	if (KEYWORDS[i] == curr) {
	    lexer->tokens.push_back(new Token{curr, TOKEN_KEYWORD});
	    return;
	}
    }
    lexer->tokens.push_back(new Token{curr, TOKEN_IDENTIFIER});
}


// TODO: Handle comments

// generates tokens for a line
void generate_tokens(Lexer* lexer)
{
    int pos = 0;
    bool currently_reading_token = false;

    std::string curr;
    Partial_Token_Type ptok;

    while (lexer->line[pos]) {
	if (!currently_reading_token) {
	    // skip whitespace
	    while (
	    lexer->line[pos] &&
	    (lexer->line[pos] == ' ' ||
	    lexer->line[pos] == '\t')
	    ) pos++;

	    currently_reading_token = true;
	    continue;
	}

	char c = lexer->line[pos];

	// reached a whitespace or end of line
	if (curr != "" && (!c || c == ' ' || c == '\t') {
	    make_token_as_per_ptok(lexer, curr, ptok);
	    curr.clear();

	    pos++;
	    currently_reading_token = false;

	    if (!c) break;
	    continue;
	}

	// handling dots in float numeric literals
	if (c == '.' && curr != "" && ptok == PTOK_NUMERIC) {
	    curr += c;
	    pos++;
	    continue;
	}

	// number
	if (is_numeric(c)) {
	    // if we start with a digit, it is numeric type
	    if (curr == "") ptok = PTOK_NUMERIC;
	    curr += c;

	    pos++;
	    continue;
	}

	// alphabet/underscore
	if (is_alpha(c) || c == '_') {

	    // RULE BREAK:
	    // invalid to have identifier starting with numbers
	    if (curr != "" && ptok == PTOK_NUMERIC) {
		fprintf(stderr, "SYNTAX ERROR: Invalid token. Identifiers cannot start with numeric characters. [line %d]", lexer->line_num);
		exit(1);
	    }

	    curr += c;
	    ptok = PTOK_ALNUM;

	    pos++;
	    continue;
	}

	// if we have reached here that means
	// (c) is some symbol. if curr is not blank
	// we must first make a token out of it, since
	// it is surely a either an identifier / keyword / numeric literal
	// which is different from whatever token the symbol will be in

	if (curr != "") {
	    make_token_as_per_ptok(lexer, curr, ptok);
	    curr.clear();
	}

	// handle the symbol
	switch (c) {
	    /* brackets */
	    case '{': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_LEFT_BRACE});
		break;
	    }
	    case '}': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_RIGHT_BRACE});
		break;
	    }
	    case '(': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_LEFT_PAREN});
		break;
	    }
	    case ')': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_RIGHT_PAREN});
		break;
	    }
	    case '[': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_LEFT_SQUARE});
		break;
	    }
	    case ']': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_RIGHT_SQUARE});
		break;
	    }
	    /* char and string literals */
	    case '\'': {
		char literal_val = lexer->line[++pos];
		if (!literal_val || literal_val == '\t') {
		    fprintf(stderr, "SYNTAX ERROR: Invalid character literal [line %d]", lexer->line_num);
		    exit(1);
		}

		lexer->tokens.push_back(new Token{std::string(1, literal_val), TOKEN_CHAR_LITERAL});

		char closing_quote = lexer->line[++pos];
		if (!closing_quote || closing_quote != '\'') {
		    fprintf(stderr, "SYNTAX ERROR: Invalid character literal. Closing quote not found. [line %d]", lexer->line_num);
		    exit(1);
		}
		break;
	    }
	    case '\"': {
		pos++;
		std::string literal;
		char literal_char;

		do {
		    literal_char = lexer->line[pos];

		    if (literal_char == '\t') {
			fprintf(stderr, "SYNTAX ERROR: Invalid character \'\\t\' in string literal [line %d]", lexer->line_num);
			exit(1);
		    }

		    literal += literal_char;
		    pos++;
		} while (literal_char && literal_char != '\"');

		if (!literal_char) {
		    fprintf(stderr, "SYNTAX ERROR: Invalid string literal. Closing quote not found. [line %d]", lexer->line_num);
		    exit(1);
		}

		lexer->tokens.push_back(new Token{literal, TOKEN_STRING_LITERAL});
		break;
	    }
	    /* other symbols */
	    case '~': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_BIN_NOT});
		break;
	    }
	    case '.': {
		lexer->tokens.push_back(new Token{NULL, TOKEN_DOT});
		break;
	    }
	}

	pos++;
    }
}

// reads the file line by line and generates tokens
void perform_lexical_analysis(Lexer* lexer, const char* file_name)
{
    std::ifstream file(file_name);
    if (!file.is_open()) {
	fprintf(stderr, "ERROR: Could not find the file");
	exit(1);
    }

    while (std::getline(file, lexer->line)) {
	lexer->line_num++;
	generate_tokens(lexer);
    }

    file.close();
}

int main()
{
    Lexer lexer;

    perform_lexical_analysis(&lexer, "program.txt");
    return 0;
}
