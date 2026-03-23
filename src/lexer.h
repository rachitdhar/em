// Lexer.h

#pragma once


#include "symbols.h"
#include "tokens.h"
#include "ast.h"
#include <fstream>
#include <iterator>
#include <stack>
#include <vector>
#include <stdint.h>


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

    smap<Data_Type *> type_info_map; // mapping type names to their data types
    void init_primitive_types();
    smap<AST_Literal *> enum_values_map; // mapping each enum to its value (ignoring the enum type)

    smap<Partial_Token *> preprocessor_definitions_map; // mapping #define definitions to actual token
    bool can_read_tokens = true; // to control reading #ifdef, #ifndef blocks
    int endif_nesting_level = 0; // is incremented when can_read_tokens is false and we see a #ifdef/#ifndef

    std::vector<std::string> libs_to_link;

    Lexer() {
	init_primitive_types();
    }
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


/* ******************** Type Information ******************* */

inline Data_Type* create_primitive_type(Primitive_Type pt) {
    return new Data_Type{ TK_PRIMITIVE, nullptr, pt };
}


// the type info map stores the mapping of type names to the Data_Type*.
// we could have defined this map globally, independent of Lexer, if it
// were only for primitive types. however, since we will also be having
// types (such as aliases created using typedefs) defined, we must consider
// the possibility that different files have the same name used as an alias
// for different actual types. so to handle that, we must associate the
// type_info_map with a particular Lexer object.
//
// for primitives (which are same everywhere), we will initialize them at
// once at the time when the lexer is created.
inline void Lexer::init_primitive_types() {
    type_info_map.insert("void", create_primitive_type(T_VOID));
    type_info_map.insert("bool", create_primitive_type(T_BOOL));

    // integer types
    type_info_map.insert("u8", create_primitive_type(T_U8));
    type_info_map.insert("u16", create_primitive_type(T_U16));
    type_info_map.insert("u32", create_primitive_type(T_U32));
    type_info_map.insert("u64", create_primitive_type(T_U64));
    type_info_map.insert("s8", create_primitive_type(T_S8));
    type_info_map.insert("s16", create_primitive_type(T_S16));
    type_info_map.insert("s32", create_primitive_type(T_S32));
    type_info_map.insert("s64", create_primitive_type(T_S64));

    type_info_map.insert("char", type_info_map["s8"]);
    type_info_map.insert("int", type_info_map["s32"]);

    // floating point types
    type_info_map.insert("f32", create_primitive_type(T_F32));
    type_info_map.insert("f64", create_primitive_type(T_F64));

    type_info_map.insert("float", type_info_map["f32"]);
    type_info_map.insert("double", type_info_map["f64"]);

    type_info_map.insert("string", create_primitive_type(T_STRING));
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
    "typedef", // can only be used in global scope
    "enum" // can only be used in global scope
};

const size_t TOTAL_KEYWORDS = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
const size_t TOTAL_DATA_TYPES = sizeof(DATA_TYPES) / sizeof(DATA_TYPES[0]);

//                Helper functions
// ************************************************

inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_numeric(char c) { return (c >= '0' && c <= '9'); }

inline char get_escape_sequence(char c)
{
    switch (c) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
	case 'b': return '\b';
	case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        case '0': return '\0';
        default: return -1; // invalid escape
    }
}

inline std::string get_filename_from_path(const std::string& path) {
    size_t pos = path.find_last_of("/\\");

    if (pos == std::string::npos)
        return path;

    return path.substr(pos + 1);
}

// to print error messages
inline void throw_error(const char *message, const std::string &line,
                        int line_num, int pos, std::string &file_name) {
    printf("%s(%d,%d): ", get_filename_from_path(file_name).c_str(), line_num, pos);
    fprintf(stderr, message);
    printf("\n\n");

    // display the line where error occurred
    printf(" %6d | %s\n", line_num, line.c_str());
    printf(" %6s | %*c\n", "", pos + 1,
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
