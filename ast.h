//
// ast.h
//


struct AST_Expression {
    // to ensure that in case a derived struct is deleted,
    // the derived struct's destructor is called
    virtual ~AST_Expression() = default;
};


struct Function_Parameter {
    std::string name;
    std::string type;
};


struct AST_Identifier : AST_Expression {
    std::string name;
};


struct AST_Literal : AST_Expression {
    std::string value;
};


union Function_Argument {
    AST_Literal literal;
    AST_Identifier identifier;
};


struct AST_Function_Definition : AST_Expression {
    std::string return_type;
    std::string function_name;
    std::vector<Function_Parameter*> params;
    std::vector<AST_Expression*> block;
};


struct AST_If_Expression : AST_Expression {
    AST_Expression *condition;
    std::vector<AST_Expression*> block;
    std::vector<AST_Expression*> else_block;
};


struct AST_For_Expression : AST_Expression {
    AST_Expression *init;
    AST_Expression *condition;
    AST_Expression *increment;
    std::vector<AST_Expression*> block;
};


struct AST_While_Expression : AST_Expression {
    AST_Expression *condition;
    std::vector<AST_Expression*> block;
};


struct AST_Declaration : AST_Expression {
    std::string data_type;
    std::string variable_name;
    AST_Expression *value;
};


struct AST_Unary_Expression : AST_Expression {
    std::string op;
    AST_Expression *expr;
};


struct AST_Binary_Expression : AST_Expression {
    std::string op;
    AST_Expression *left;
    AST_Expression *right;
};


struct AST_Function_Call : AST_Expression {
    std::string function_name;
    std::vector<Function_Argument> params;
};


struct AST_Return_Expression : AST_Expression {
    AST_Expression *value;
};


struct AST_Jump_Expression : AST_Expression {
    std::string jump_type;
};
