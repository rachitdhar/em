//
// symbols.h
//

#ifndef SYMBOLS_H
#define SYMBOLS_H


#include "types.h"
#include "dsa.h"


//                       Symbol Table
// ********************************************************

/*
a symbol table to store the information on
variables and functions that are encountered
during the parsing stage.

This will be useful for:

    (1) checking whether a variable being used
    has been declared previously.

    (2) whether a variable is not being redeclared.

    (3) performing type checking (after parsing is complete)

in order to handle the scope, we will need to make
use of a "stack" of symbol tables. whenever we enter a
new nested scope, we will push a new entry into the stack,
and upon leaving a scope, we will pop the top of the stack.
since i am planning on iterating through this stack, i am not
using a real stack, but instead a doubly linked list.
*/

enum Symbol_Type { SYM_VARIABLE, SYM_FUNCTION };

struct Symbol {
    std::string identifier;
    Symbol_Type symbol_type;
    bool is_declaration = true;
    Data_Type *return_type = nullptr; // data_type in case of variables
    std::vector<Data_Type *> *signature = NULL; // param types (only for functions)
    bool has_variadic_args = false; // (only for functions)
};

struct Scope {
    struct Scope *parent = NULL;
    struct Scope *child = NULL;
    smap<Symbol *> variables;
};

struct Symbol_Table {
    Scope *head_scope = NULL;
    Scope *curr_scope = NULL;

    smap<Symbol *> global_variables;
    smap<Symbol *> functions;
    smap<Symbol *> function_prototypes;

    void push();                 // pushes a new scope
    void pop();                  // pops the topmost scope
    void insert(Symbol *symbol); // inserts a symbol into the current scope
    bool exists(std::string name,
                Symbol_Type symbol_type); // checks if symbol exists
    bool prototype_exists(
        std::string name); // tells whether a function prototype exists
    Data_Type *get_return_type(
        std::string name, Symbol_Type symbol_type); // returns the return type of the symbol
};


inline void Symbol_Table::push() {
    auto *scope = new Scope;

    if (curr_scope == NULL) {
        head_scope = scope;
        curr_scope = scope;
        return;
    }

    curr_scope->child = scope;
    scope->parent = curr_scope;
    curr_scope = scope;
}

inline void Symbol_Table::pop() {
    if (curr_scope->child != NULL)
        fprintf(stderr, "ERROR (Fatal): Failed to exit a scope.");

    // remove all symbols inside this scope
    for (size_t i = 0; i < curr_scope->variables.capacity; i++) {
        if (curr_scope->variables.data[i].occupied &&
            !curr_scope->variables.data[i].deleted)
            delete curr_scope->variables.data[i].value;
    }

    Scope *parent = curr_scope->parent;

    delete curr_scope;
    if (parent != NULL)
        parent->child = NULL;
    curr_scope = parent;
}

inline void Symbol_Table::insert(Symbol *symbol) {
    if (symbol->symbol_type == SYM_VARIABLE) {
        if (curr_scope == NULL) {
            global_variables.insert(symbol->identifier, symbol);
            return;
        }
        curr_scope->variables.insert(symbol->identifier, symbol);
    } else {
        functions.insert(symbol->identifier, symbol);
    }
}

inline bool Symbol_Table::exists(std::string name, Symbol_Type symbol_type) {
    // if it is a function, we just need to check the functions map
    if (symbol_type == SYM_FUNCTION) {
        return functions[name] != NULL;
    }

    // first check global variables
    if (global_variables[name] != NULL)
        return true;

    // we will go backwards, starting from the innermost scope
    // to find the symbol

    auto *scope_to_search = curr_scope;

    while (scope_to_search != NULL) {
        if (scope_to_search->variables[name] != NULL)
            return true;

        scope_to_search = scope_to_search->parent;
    }
    return false;
}

inline bool Symbol_Table::prototype_exists(std::string name) {
    return function_prototypes[name] != NULL;
}

inline Data_Type *Symbol_Table::get_return_type(std::string name, Symbol_Type symbol_type) {
    // if it is a function, we just need to check the functions map
    if (symbol_type == SYM_FUNCTION) {
        if (functions[name] == NULL) return nullptr;

	return functions[name]->return_type;
    }

    // first check global variables
    if (global_variables[name] != NULL) {
	return global_variables[name]->return_type;
    }

    // we will go backwards, starting from the innermost scope
    // to find the symbol

    auto *scope_to_search = curr_scope;

    while (scope_to_search != NULL) {
        if (scope_to_search->variables[name] != NULL) {
	    return scope_to_search->variables[name]->return_type;
	}

        scope_to_search = scope_to_search->parent;
    }
    return nullptr;
}

#endif
