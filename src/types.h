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
    Data_Type* dt = data_type;
    while (dt->type_kind == TK_ALIAS) dt = dt->base_type;

    if (dt->type_kind != TK_PRIMITIVE) {
	fprintf(stderr, "TYPE ERROR: Expected primitive data type.");
	exit(1);
    }
    return (dt->name.p >= T_U8 && dt->name.p <= T_S64);
}


const std::string DATA_TYPES[] = {
    "void", "bool", "int",
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "float", "f32", "double", "f64",
    "char", "string"
};

#endif
