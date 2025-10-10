//
// Lexer.cpp
//

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


// handle preprocessor directives during lexical analysis
void handle_preprocessor_directive(Lexer *lexer, std::string preprocessor_directive_name, int pos)
{
    if (preprocessor_directive_name == "import") {
	// the next token should be a string containing the file path

	// skip whitespace
	while (
	lexer->line[pos] &&
	(lexer->line[pos] == ' ' ||
	lexer->line[pos] == '\t')
	) pos++;

	if (!lexer->line[pos] || lexer->line[pos] != '\"') {
	    throw_error(
	    "SYNTAX ERROR: import file path not specified.",
	    lexer->line, lexer->line_num, pos, lexer->file_name);
	}
	pos++;

	std::string import_file_name;
	bool is_valid_string = false;

	while (lexer->line[pos]) {
	    if (lexer->line[pos] == '\"') {
		is_valid_string = true;
		break;
	    }
	    import_file_name += lexer->line[pos];
	    pos++;
	}

	if (!is_valid_string) {
	    throw_error(
	    "SYNTAX ERROR: Invalid string provided for import file path.",
	    lexer->line, lexer->line_num, pos, lexer->file_name);
	}

	// tokenize the import file specified
	Lexer *import_lexer = perform_lexical_analysis(import_file_name.c_str());

	/*
	here is how we want to handle imports. essentially we just
	want to return a single lexer object for a file, irrespective
	of how many imports it has which are individually being tokenized.

	so to handle that, we will take the token vectors from each individual
	import, and push those tokens into our main lexer. now, to ensure that
	we can provide the correct syntax error messages (with the actual file
	name from where those tokens originated) we need to track the file name
	from where the token actually came. so we are storing the file name
	in the tokens themselves for this reason. wherever these tokens go
	they will maintain the file name of their source.
	*/

	// push the imported file tokens into our main lexer.
	// we will not copy the tokens, but instead move them directly.
	lexer->tokens.insert(
	    lexer->tokens.end(),
	    std::make_move_iterator(import_lexer->tokens.begin()),
	    std::make_move_iterator(import_lexer->tokens.end())
	);

	lexer->total_lines_postprocessing += import_lexer->total_lines_postprocessing;
	return;
    }

    // throw an error if invalid directive passed
    throw_error(
    "SYNTAX ERROR: Invalid preprocessor directive encountered.",
    lexer->line, lexer->line_num, pos, lexer->file_name);
}


inline void make_token_as_per_ptok(Lexer* lexer, std::string& curr, Partial_Token_Type ptok, int pos)
{
    if (ptok == PTOK_NUMERIC) {
	lexer->tokens.push_back(Token{curr, TOKEN_NUMERIC_LITERAL, lexer->line_num, pos, lexer->file_name});
	return;
    }

    if (curr == "true" || curr == "false") {
	lexer->tokens.push_back(Token{curr, TOKEN_BOOL_LITERAL, lexer->line_num, pos, lexer->file_name});
	return;
    }

    for (int i = 0; i < TOTAL_KEYWORDS; i++) {
	if (KEYWORDS[i] == curr) {
	    lexer->tokens.push_back(Token{curr, TOKEN_KEYWORD, lexer->line_num, pos, lexer->file_name});
	    return;
	}
    }

    for (int i = 0; i < TOTAL_DATA_TYPES; i++) {
	if (DATA_TYPES[i] == curr) {
	    lexer->tokens.push_back(Token{curr, TOKEN_DATA_TYPE, lexer->line_num, pos, lexer->file_name});
	    return;
	}
    }
    lexer->tokens.push_back(Token{curr, TOKEN_IDENTIFIER, lexer->line_num, pos, lexer->file_name});
}


// generates tokens for a line
// it returns a boolean telling whether we are inside a multiline comment
// at the start of the next line
bool generate_tokens(Lexer* lexer, bool inside_multiline_comment)
{
    int pos = 0;
    bool currently_reading_token = false;
    bool encountered_comment = false;

    std::string curr;
    Partial_Token_Type ptok;

    while (lexer->line[pos] && !encountered_comment) {
	if (!currently_reading_token) {
	    // if in a multiline comment, keep reading
	    // until we reach '*/'

	    if (inside_multiline_comment) {
		while (lexer->line[pos]) {
		    if (lexer->line[pos] == '*') {
			pos++;
			if (lexer->line[pos] && lexer->line[pos] == '/') {
			    inside_multiline_comment = false;
			    pos++;
			    break;
			}
		    }
		    pos++;
		}
	    }

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

	// handling preprocessor directives
	if (c == '#') {
	    pos++;

	    // read the name of the directive
	    std::string preprocessor_directive_name;
	    while (lexer->line[pos] && lexer->line[pos] != ' ' && lexer->line[pos] != '\t') {
	        preprocessor_directive_name += lexer->line[pos];
		pos++;
	    }

	    handle_preprocessor_directive(lexer, preprocessor_directive_name, pos);
	    break; // ignore the rest of the line after this (if anything exists)
	}

	// reached a whitespace or end of line
	if (curr != "" && (!c || c == ' ' || c == '\t')) {
	    make_token_as_per_ptok(lexer, curr, ptok, pos);
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
		throw_error(
		"SYNTAX ERROR: Invalid token. Identifiers cannot start with numeric characters.",
		lexer->line, lexer->line_num, pos, lexer->file_name);
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
	    make_token_as_per_ptok(lexer, curr, ptok, pos);
	    curr.clear();
	}

	// handle the symbol
	switch (c) {
	    /* brackets */
	    case '{': {
		lexer->tokens.push_back(Token{"{", TOKEN_LEFT_BRACE, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case '}': {
		lexer->tokens.push_back(Token{"}", TOKEN_RIGHT_BRACE, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case '(': {
		lexer->tokens.push_back(Token{"(", TOKEN_LEFT_PAREN, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case ')': {
		lexer->tokens.push_back(Token{")", TOKEN_RIGHT_PAREN, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case '[': {
		lexer->tokens.push_back(Token{"[", TOKEN_LEFT_SQUARE, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case ']': {
		lexer->tokens.push_back(Token{"]", TOKEN_RIGHT_SQUARE, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    /* char and string literals */
	    case '\'': {
		char literal_val = lexer->line[++pos];
		if (!literal_val || literal_val == '\t') {
		    throw_error(
		    "SYNTAX ERROR: Invalid character literal",
		    lexer->line, lexer->line_num, pos, lexer->file_name);
		}

		lexer->tokens.push_back(Token{std::string(1, literal_val), TOKEN_CHAR_LITERAL, lexer->line_num, pos, lexer->file_name});

		char closing_quote = lexer->line[++pos];
		if (!closing_quote || closing_quote != '\'') {
		    throw_error(
		    "SYNTAX ERROR: Invalid character literal. Closing quote not found.",
		    lexer->line, lexer->line_num, pos, lexer->file_name);
		}
		pos++;
		break;
	    }
	    case '\"': {
		pos++;
		std::string literal;
		char literal_char = lexer->line[pos];

		while (literal_char && literal_char != '\"') {
		    if (literal_char == '\t') {
			throw_error(
			"SYNTAX ERROR: Invalid character \'\\t\' in string literal",
			lexer->line, lexer->line_num, pos, lexer->file_name);
		    }

		    literal += literal_char;
		    pos++;
		    literal_char = lexer->line[pos];
		}

		if (!literal_char) {
		    throw_error(
		    "SYNTAX ERROR: Invalid string literal. Closing quote not found.",
		    lexer->line, lexer->line_num, pos, lexer->file_name);
		}

		lexer->tokens.push_back(Token{literal, TOKEN_STRING_LITERAL, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    /* other symbols */
	    case '~': {
		lexer->tokens.push_back(Token{"~", TOKEN_BIT_NOT, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case '.': {
	      lexer->tokens.push_back(Token{".", TOKEN_DOT, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case ',': {
		lexer->tokens.push_back(Token{",", TOKEN_SEPARATOR, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    case ';': {
		lexer->tokens.push_back(Token{";", TOKEN_DELIMITER, lexer->line_num, pos, lexer->file_name});
		pos++;
		break;
	    }
	    /* (! !=) */
	    case '!': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"!=", TOKEN_NOTEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"!", TOKEN_NOT, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (+ ++ +=)  */
	    case '+': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"+=", TOKEN_PLUSEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '+') {
		    lexer->tokens.push_back(Token{"++", TOKEN_INCREMENT, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"+", TOKEN_PLUS, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (- -- -=) */
	    case '-': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"-=", TOKEN_MINUSEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '-') {
		    lexer->tokens.push_back(Token{"--", TOKEN_DECREMENT, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"-", TOKEN_MINUS, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (* *=) */
	    case '*': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"*=", TOKEN_MULTIPLYEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"*", TOKEN_STAR, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (/ /= // /*) */
	    case '/': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"/=", TOKEN_DIVIDEEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '/') {
		    // encountered a comment
		    // (so the remaining line ahead will be ignored)

		    encountered_comment = true;
		    break;
		} else if (next && next == '*') {
		    // encountered a multi-line comment

		    inside_multiline_comment = true;
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"/", TOKEN_DIVIDE, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (% %=) */
	    case '%': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"%=", TOKEN_MODEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"%", TOKEN_MOD, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (< <= << <<=) */
	    case '<': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"<=", TOKEN_LESSEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '<') {
		    next = lexer->line[++pos];
		    if (next && next == '=') {
			lexer->tokens.push_back(Token{"<<=", TOKEN_LSHIFT_EQ, lexer->line_num, pos, lexer->file_name});
			pos++;
			break;
		    }
		    lexer->tokens.push_back(Token{"<<", TOKEN_LSHIFT, lexer->line_num, pos, lexer->file_name});
		    break;
		}
		lexer->tokens.push_back(Token{"<", TOKEN_LESS, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (> >= >> >>=) */
	    case '>': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{">=", TOKEN_GREATEREQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '>') {
		    next = lexer->line[++pos];
		    if (next && next == '=') {
			lexer->tokens.push_back(Token{">>=", TOKEN_RSHIFT_EQ, lexer->line_num, pos, lexer->file_name});
			pos++;
			break;
		    }
		    lexer->tokens.push_back(Token{">>", TOKEN_RSHIFT, lexer->line_num, pos, lexer->file_name});
		    break;
		}
		lexer->tokens.push_back(Token{">", TOKEN_GREATER, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (= ==) */
	    case '=': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"==", TOKEN_EQUAL, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"=", TOKEN_ASSIGN, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (& && &= &&=) */
	    case '&': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"&=", TOKEN_BIT_ANDEQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '&') {
		    next = lexer->line[++pos];
		    if (next && next == '=') {
			lexer->tokens.push_back(Token{"&&=", TOKEN_ANDEQ, lexer->line_num, pos, lexer->file_name});
			pos++;
			break;
		    }
		    lexer->tokens.push_back(Token{"&&", TOKEN_AND, lexer->line_num, pos, lexer->file_name});
		    break;
		}
		lexer->tokens.push_back(Token{"&", TOKEN_AMPERSAND, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (| || |= ||=) */
	    case '|': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"|=", TOKEN_BIT_OREQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		} else if (next && next == '|') {
		    next = lexer->line[++pos];
		    if (next && next == '=') {
			lexer->tokens.push_back(Token{"||=", TOKEN_OREQ, lexer->line_num, pos, lexer->file_name});
			pos++;
			break;
		    }
		    lexer->tokens.push_back(Token{"||", TOKEN_OR, lexer->line_num, pos, lexer->file_name});
		    break;
		}
		lexer->tokens.push_back(Token{"|", TOKEN_BIT_OR, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    /* (^ ^=) */
	    case '^': {
		pos++;
		char next = lexer->line[pos];

		if (next && next == '=') {
		    lexer->tokens.push_back(Token{"^=", TOKEN_XOREQ, lexer->line_num, pos, lexer->file_name});
		    pos++;
		    break;
		}
		lexer->tokens.push_back(Token{"^", TOKEN_XOR, lexer->line_num, pos, lexer->file_name});
		break;
	    }
	    default: {
		throw_error(
		"SYNTAX ERROR: Invalid token encountered.",
		lexer->line, lexer->line_num, pos, lexer->file_name);
	    }
	}

	currently_reading_token = false; // to handle possible whitespace
    }

    return inside_multiline_comment;
}

// reads the file line by line and generates tokens
Lexer *perform_lexical_analysis(const char* file_name)
{
    auto *lexer = new Lexer;

    std::ifstream file(file_name);
    if (!file.is_open()) {
	fprintf(stderr, "ERROR: Could not find the file: %s", file_name);
	exit(1);
    }

    lexer->file_name = file_name;
    bool inside_multiline_comment = false;

    while (std::getline(file, lexer->line)) {
	lexer->line_num++;
	inside_multiline_comment = generate_tokens(lexer, inside_multiline_comment);
    }
    lexer->total_lines_postprocessing += lexer->line_num;

    file.close();
    return lexer;
}
