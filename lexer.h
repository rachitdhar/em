// Lexer.h

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>


enum Token_Type {
    TOKEN_IDENTIFIER = 0,          // abcd  sdf2324  dfs_23fs56df
    TOKEN_KEYWORD = 1,
    TOKEN_DATA_TYPE = 2,
    TOKEN_NUMERIC_LITERAL = 3,     // 23434 12.656
    TOKEN_CHAR_LITERAL = 4,        // 'A'
    TOKEN_STRING_LITERAL = 5,      // "fds..."
    TOKEN_SEPARATOR = 6,           // ,
    TOKEN_DELIMITER = 7,           // ;

    /* brackets */
    TOKEN_LEFT_BRACE = 100,    // {
    TOKEN_RIGHT_BRACE,         // }
    TOKEN_LEFT_PAREN,          // (
    TOKEN_RIGHT_PAREN,         // )
    TOKEN_LEFT_SQUARE,         // [
    TOKEN_RIGHT_SQUARE,        // ]

    /* unary ops */
    TOKEN_NOT = 200,           // !
    TOKEN_BIT_NOT,             // ~
    TOKEN_INCREMENT,           // ++
    TOKEN_DECREMENT,           // --

    /* binary ops */
    TOKEN_PLUS = 300,          // +
    TOKEN_MINUS,               // -
    TOKEN_DIVIDE,              // /
    TOKEN_MOD,                 // %
    TOKEN_PLUSEQ,              // +=
    TOKEN_MINUSEQ,             // -=
    TOKEN_MULTIPLYEQ,          // *=
    TOKEN_DIVIDEEQ,            // /=
    TOKEN_MODEQ,               // %=
    TOKEN_LESS,                // <
    TOKEN_GREATER,             // >
    TOKEN_LESSEQ,              // <=
    TOKEN_GREATEREQ,           // >=
    TOKEN_ASSIGN,              // =
    TOKEN_EQUAL,               // ==
    TOKEN_NOTEQ,               // !=
    TOKEN_AND,                 // &&
    TOKEN_OR,                  // ||
    TOKEN_BIT_OR,              // |
    TOKEN_XOR,                 // ^
    TOKEN_ANDEQ,               // &&=
    TOKEN_OREQ,                // ||=
    TOKEN_BIT_ANDEQ,           // &=
    TOKEN_BIT_OREQ,            // |=
    TOKEN_XOREQ,               // ^=
    TOKEN_DOT,                 // .

    /* non context-free (must be interpreted at parsing) */
    TOKEN_STAR = 400,          // * (multiplication / pointer de-reference)
    TOKEN_AMPERSAND,           // & (binary and / address-of)
};


inline bool is_literal(Token *tok)
{
    return (tok->type >= 3 && tok->type <= 5);
}

inline bool is_unary_op(Token *tok)
{
    return (tok->type >= 200 && tok->type < 300);
}

inline bool is_binary_op(Token *tok)
{
    return (tok->type >= 300);
}


// partial types
// (for raw strings, before having a determined token)

enum Partial_Token_Type {
    PTOK_NUMERIC,     // pure numbers
    PTOK_ALNUM,       // both alphabets and numbers (starts with alpha)
};


struct Token {
    std::string val;
    Token_Type type;

    int line_num;
    int position;
};

struct Lexer {
    std::string line;
    int line_num = 0;

    std::vector<Token> tokens;

    /* for parsing */
    int curr_token_index = 0;

    Token *get_next_token();          // moves to next token, and returns it
    Token *peek_next_token();         // returns the next token
    Token *peek(int num_tokens_ahead);// returns a token (n) steps ahead
    Token *peek_prev_token();         // returns the prev token
    void move_to_next_token();        // moves to the next token
};


inline Token* Lexer::get_next_token()
{
    if (curr_token_index + 1 >= tokens.size()) return NULL;
    return &(tokens[++curr_token_index]);
}

inline Token* Lexer::peek_next_token()
{
    if (curr_token_index + 1 >= tokens.size()) return NULL;
    return &(tokens[curr_token_index + 1]);
}

inline Token* Lexer::peek(int num_tokens_ahead)
{
    if (curr_token_index + num_tokens_ahead >= tokens.size()) return NULL;
    return &(tokens[curr_token_index + num_tokens_ahead]);
}

inline Token* Lexer::peek_prev_token()
{
    if (curr_token_index <= 0) return NULL;
    return &(tokens[curr_token_index - 1]);
}

inline void Lexer::move_to_next_token()
{
    if (curr_token_index + 1 >= tokens.size()) return NULL;
    curr_token_index++;
}


//          list of keywords for the language
// *****************************************************

const std::string DATA_TYPES[] = {
    "void",
    "int",
    "float",
    "char",
    "string"
};

const std::string KEYWORDS[] = {
    /* control flow */
    "if",
    "else",
    "for",
    "while",

    /* jumps */
    "return",
    "break",
    "continue",
};

const size_t TOTAL_KEYWORDS = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
const size_t TOTAL_DATA_TYPES = sizeof(DATA_TYPES) / sizeof(DATA_TYPES[0]);

//                Helper functions
// ************************************************

inline bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_numeric(char c)
{
    return (c >= '0' && c <= '9');
}

inline void make_token_as_per_ptok(Lexer* lexer, std::string& curr, Partial_Token_Type ptok, int pos)
{
    if (ptok == PTOK_NUMERIC) {
	lexer->tokens.push_back(Token{curr, TOKEN_NUMERIC_LITERAL, lexer->line_num, pos});
	return;
    }

    for (int i = 0; i < TOTAL_KEYWORDS; i++) {
	if (KEYWORDS[i] == curr) {
	    lexer->tokens.push_back(Token{curr, TOKEN_KEYWORD, lexer->line_num, pos});
	    return;
	}
    }

    for (int i = 0; i < TOTAL_DATA_TYPES; i++) {
	if (DATA_TYPES[i] == curr) {
	    lexer->tokens.push_back(Token{curr, TOKEN_DATA_TYPE, lexer->line_num, pos});
	    return;
	}
    }
    lexer->tokens.push_back(Token{curr, TOKEN_IDENTIFIER, lexer->line_num, pos});
}

// to print error messages
inline void throw_error(const char *message, std::string line, int line_num, int pos)
{
    fprintf(stderr, message);
    printf(" [line %d, position %d]\n\n", line_num, pos);

    // display the line where error occurred
    printf("\t%s\n", line.c_str());
    printf("\t%*c\n", pos + 1, '^'); // to mark (using ^) the position of the error

    exit(1); // terminate the execution
}

// (For Debugging)
//
// to print the list of tokens generated by the lexer
inline void print_tokens(Lexer* lexer)
{
    for (Token& tok : lexer->tokens) {
	printf("<\'%s\', %d>\n", tok.val.c_str(), tok.type);
    }
}

//               Function definitions
// **************************************************
void perform_lexical_analysis(Lexer* lexer, const char* file_name);

#endif
