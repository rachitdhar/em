//
// parser.h
//

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

    printf(" [line %d, position %d]", tok->line_num, tok->position);
    exit(1);
}


// insufficient token errors
inline void throw_error__insufficient_tokens_func_def(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Insufficient tokens for function definition.", lexer);
}
