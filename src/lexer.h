// Lexer.h

#ifndef LEXER_H
#define LEXER_H


#include "errors.h"
#include "linker.h"
#include "symbols.h"
#include <fstream>
#include <iterator>
#include <stack>
#include <vector>
#include <stdint.h>


enum Token_Type {
    TOKEN_NONE = 0, // NO TOKEN ASSIGNED (not a real token)

    TOKEN_IDENTIFIER = 1, // abcd  sdf2324  dfs_23fs56df
    TOKEN_KEYWORD = 2,
    TOKEN_DATA_TYPE = 3,
    TOKEN_NUMERIC_LITERAL = 4, // 23434 12.656
    TOKEN_CHAR_LITERAL = 5,    // 'A'
    TOKEN_STRING_LITERAL = 6,  // "fds..."
    TOKEN_BOOL_LITERAL = 7,    // true, false
    TOKEN_SEPARATOR = 8,       // ,
    TOKEN_DELIMITER = 9,       // ;
    TOKEN_COLON = 10,          // :

    /* brackets */
    TOKEN_LEFT_BRACE = 100, // {
    TOKEN_RIGHT_BRACE,      // }
    TOKEN_LEFT_PAREN,       // (
    TOKEN_RIGHT_PAREN,      // )
    TOKEN_LEFT_SQUARE,      // [
    TOKEN_RIGHT_SQUARE,     // ]

    /* unary ops */
    TOKEN_NOT = 200, // !
    TOKEN_BIT_NOT,   // ~
    TOKEN_INCREMENT, // ++
    TOKEN_DECREMENT, // --

    /* binary ops */
    TOKEN_PLUS = 300, // +
    TOKEN_MINUS,      // -
    TOKEN_DIVIDE,     // /
    TOKEN_MOD,        // %
    TOKEN_PLUSEQ,     // +=
    TOKEN_MINUSEQ,    // -=
    TOKEN_MULTIPLYEQ, // *=
    TOKEN_DIVIDEEQ,   // /=
    TOKEN_MODEQ,      // %=
    TOKEN_LESS,       // <
    TOKEN_GREATER,    // >
    TOKEN_LESSEQ,     // <=
    TOKEN_GREATEREQ,  // >=
    TOKEN_LSHIFT,     // <<
    TOKEN_RSHIFT,     // >>
    TOKEN_LSHIFT_EQ,  // <<=
    TOKEN_RSHIFT_EQ,  // >>=
    TOKEN_ASSIGN,     // =
    TOKEN_EQUAL,      // ==
    TOKEN_NOTEQ,      // !=
    TOKEN_AND,        // &&
    TOKEN_OR,         // ||
    TOKEN_BIT_OR,     // |
    TOKEN_XOR,        // ^
    TOKEN_ANDEQ,      // &&=
    TOKEN_OREQ,       // ||=
    TOKEN_BIT_ANDEQ,  // &=
    TOKEN_BIT_OREQ,   // |=
    TOKEN_XOREQ,      // ^=
    TOKEN_DOT,        // .

    /* non context-free (must be interpreted at parsing) */
    TOKEN_STAR = 400, // * (multiplication / pointer de-reference)
    TOKEN_AMPERSAND,  // & (binary and / address-of)
};

// partial types
// (for raw strings, before having a determined token)

enum Partial_Token_Type {
    PTOK_NUMERIC, // pure numbers
    PTOK_ALNUM,   // both alphabets and numbers (starts with alpha)
};

// only using this for #define definition mappings
struct Partial_Token {
    std::string val;
    Partial_Token_Type type = PTOK_ALNUM;
    std::string file_name;
};

struct Token {
    std::string val;
    Token_Type type = TOKEN_NONE;

    int line_num;
    int position;
    std::string file_name;
};

inline bool is_literal(Token *tok) {
    return (tok->type >= 4 && tok->type <= 7);
}

inline bool is_unary_op(Token *tok) {
    return (tok->type >= 200 && tok->type < 300);
}

inline bool is_binary_op(Token *tok) { return (tok->type >= 300); }

//                           Lexer
// ***********************************************************

struct Lexer {
    std::string file_name;
    std::string line;
    int line_num = 0;
    int total_lines_postprocessing = 0;

    std::vector<Token> tokens;

    /* for parsing */
    int curr_token_index = 0;

    Token *get_next_token();           // moves to next token, and returns it
    Token *peek_next_token();          // returns the next token
    Token *peek(int num_tokens_ahead); // returns a token (n) steps ahead
    Token *peek_prev_token();          // returns the prev token
    void move_to_next_token();         // moves to the next token

    Symbol_Table symbol_table; // to store symbols during parsing
    bool entry_point_found = false;

    smap<Partial_Token *> preprocessor_definitions_map; // mapping #define definitions to actual token
    bool can_read_tokens = true; // to control reading #ifdef, #ifndef blocks
    int endif_nesting_level = 0; // is incremented when can_read_tokens is false and we see a #ifdef/#ifndef

    std::vector<std::string> libs_to_link;
};

inline Token *Lexer::get_next_token() {
    if (curr_token_index + 1 >= tokens.size())
        return NULL;
    return &(tokens[++curr_token_index]);
}

inline Token *Lexer::peek_next_token() {
    if (curr_token_index + 1 >= tokens.size())
        return NULL;
    return &(tokens[curr_token_index + 1]);
}

inline Token *Lexer::peek(int num_tokens_ahead = 0) {
    if (curr_token_index + num_tokens_ahead >= tokens.size())
        return NULL;
    return &(tokens[curr_token_index + num_tokens_ahead]);
}

inline Token *Lexer::peek_prev_token() {
    if (curr_token_index <= 0)
        return NULL;
    return &(tokens[curr_token_index - 1]);
}

inline void Lexer::move_to_next_token() {
    if (curr_token_index + 1 >= tokens.size())
        return;
    curr_token_index++;
}

//          list of keywords for the language
// *****************************************************

const std::string KEYWORDS[] = {
    /* control flow */
    "if",
    "else",
    "switch",
    "case",
    "for",
    "while",

    /* jumps */
    "return",
    "break",
    "continue",

    /* special builtins */
    "varg",
};

const size_t TOTAL_KEYWORDS = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
const size_t TOTAL_DATA_TYPES = sizeof(DATA_TYPES) / sizeof(DATA_TYPES[0]);

//                Helper functions
// ************************************************

inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_numeric(char c) { return (c >= '0' && c <= '9'); }

// to print error messages
inline void throw_error(const char *message, const std::string &line,
                        int line_num, int pos, std::string &file_name) {
    printf("[%s: line %d, position %d] ", file_name.c_str(), line_num, pos);
    fprintf(stderr, message);
    printf("\n\n");

    // display the line where error occurred
    printf("\t%s\n", line.c_str());
    printf("\t%*c\n", pos + 1,
           '^'); // to mark (using ^) the position of the error

    exit(1); // terminate the execution
}

// (For Debugging)
//
// to print the list of tokens generated by the lexer
inline void print_tokens(Lexer *lexer) {
    for (Token &tok : lexer->tokens) {
        printf("<\'%s\', %d>\n", tok.val.c_str(), tok.type);
    }
}

//               Function definitions
// **************************************************
Lexer *perform_lexical_analysis(const char *file_name);

#endif
