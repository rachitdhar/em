//
// types.h
//

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <string>


enum Type_Kind {
   TK_PRIMITIVE,
   TK_POINTER,
   TK_ARRAY,
   TK_ENUM,
   TK_STRUCT,
   TK_ALIAS
};

// to store at the parsing stage (in the ast)
enum Primitive_Type {
    T_VOID,
    T_BOOL,

    /* int types begin */
    T_U8,
    T_U16,
    T_U32,
    T_U64,
    T_S8,
    T_S16,
    T_S32,
    T_S64,
    /* int types end */

    T_F32,
    T_F64,
    T_STRING
};

struct Data_Type {
   Type_Kind type_kind = TK_PRIMITIVE;
   Data_Type *base_type = nullptr; // useful for pointers

   union {
       Primitive_Type p; // for primitive types
       std::string *np;  // for non-primitive types
   } name;
};


inline bool is_int_type(Data_Type *data_type) {
    if (data_type->type_kind != TK_PRIMITIVE) {
	fprintf(stderr, "TYPE ERROR: Expected primitive data time");
	exit(1);
    }
    return (data_type->name.p >= T_U8 && data_type->name.p <= T_S64);
}


const std::string DATA_TYPES[] = {
    "void", "bool", "int",
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "float", "f32", "double", "f64",
    "char", "string"
};


inline Data_Type* create_primitive_type(Primitive_Type pt) {
    return new Data_Type{ TK_PRIMITIVE, nullptr, pt };
}


// to create and return the primitive types during parsing
inline Data_Type* type_map(const std::string& type) {
    if (type == "void")
        return create_primitive_type(T_VOID);
    if (type == "bool")
        return create_primitive_type(T_BOOL);
    if (type == "u8")
        return create_primitive_type(T_U8);
    if (type == "u16")
        return create_primitive_type(T_U16);
    if (type == "u32")
        return create_primitive_type(T_U32);
    if (type == "u64")
        return create_primitive_type(T_U64);
    if (type == "char" || type == "s8")
        return create_primitive_type(T_S8);
    if (type == "s16")
        return create_primitive_type(T_S16);
    if (type == "int" || type == "s32")
        return create_primitive_type(T_S32);
    if (type == "s64")
        return create_primitive_type(T_S64);
    if (type == "float" || type == "f32")
        return create_primitive_type(T_F32);
    if (type == "double" || type == "f64")
        return create_primitive_type(T_F64);
    if (type == "string")
        return create_primitive_type(T_STRING);

    return create_primitive_type(T_VOID);
}


#endif
