#include "ensnare/private/type.hpp"

using namespace ensnare;

ensnare::PtrType::PtrType(Type pointee) : pointee(pointee) {}

ensnare::RefType::RefType(Type pointee) : pointee(pointee) {}

ensnare::InstType::InstType(Sym name, Vec<Type> types) : name(name), types(types) {}

ensnare::UnsizedArrayType::UnsizedArrayType(Type type) : type(type) {}

ensnare::ArrayType::ArrayType(Expr size, Type type) : size(size), type(type) {}

ensnare::FuncType::FuncType(Vec<Type> params, Type return_type)
   : params(params), return_type(return_type) {}

ensnare::ConstType::ConstType(Type type) : type(type) {}
