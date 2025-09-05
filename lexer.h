// Lexer.h

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>


enum Token_Type {
    TOKEN_IDENTIFIER,          // abcd  sdf2324  dfs_23fs56df
    TOKEN_KEYWORD,
    TOKEN_NUMERIC_LITERAL,     // 23434
    TOKEN_CHAR_LITERAL,        // 'A'
    TOKEN_STRING_LITERAL,      // "fds..."
    TOKEN_SYMBOL,              // , ; #

    /* brackets */
    TOKEN_LEFT_BRACE,          // {
    TOKEN_RIGHT_BRACE,         // }
    TOKEN_LEFT_PAREN,          // (
    TOKEN_RIGHT_PAREN,         // )
    TOKEN_LEFT_SQUARE,         // [
    TOKEN_RIGHT_SQUARE,        // ]

    /* unary ops */
    TOKEN_NOT,                 // !
    TOKEN_BIN_NOT,             // ~
    TOKEN_INCREMENT,           // ++
    TOKEN_DECREMENT,           // --

    /* binary ops */
    TOKEN_PLUS,                // +
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
    TOKEN_XOR,                 // ^
    TOKEN_ANDEQ,               // &&=
    TOKEN_OREQ,                // ||=
    TOKEN_XOREQ,               // ^=
    TOKEN_DOT,                 // .

    /* non context-free (must be interpreted at parsing) */
    TOKEN_STAR,                // * (multiplication / pointer de-reference)
    TOKEN_AMPERSAND,           // & (binary and / address-of)
};

struct Token {
    std::string val;
    Token_Type type;
};

struct Lexer {
    std::string line;
    int line_num;

    std::vector<Token> tokens;
};

//          list of keywords for the language
// *****************************************************

std::string KEYWORDS[] = {
    /* data types */
    "void",
    "uint",
    "char",
    "string",

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

#endif
