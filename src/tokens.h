//
// tokens.h
//

#pragma once

#include <string>

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
