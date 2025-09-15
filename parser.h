//
// parser.h
//


#include <unordered_map>


// defining precedence levels
// (from lowest to highest)
enum Precedence {
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
    {TOKEN_NUMERIC_LITERAL, PREC_PRIMARY},
    {TOKEN_CHAR_LITERAL,    PREC_PRIMARY},
    {TOKEN_STRING_LITERAL,  PREC_PRIMARY},
    {TOKEN_LEFT_PAREN,      PREC_PRIMARY},
    {TOKEN_RIGHT_PAREN,     PREC_PRIMARY}
};



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


// missing delimiter ';' at end of statement
inline void throw_error__missing_delimiter(Lexer *lexer)
{
    throw_parser_error("SYNTAX ERROR: Missing delimiter \';\' at the end of the statement.", lexer);
}
