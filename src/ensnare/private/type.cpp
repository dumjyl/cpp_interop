#include "ensnare/private/type.hpp"

using namespace ensnare;

ensnare::PtrType::PtrType(Type pointee) : pointee(pointee) {}

ensnare::RefType::RefType(Type pointee) : pointee(pointee) {}

ensnare::InstType::InstType(Type type, Vec<Arg> args) : type(type), args(args) {}

ensnare::UnsizedArrayType::UnsizedArrayType(Type type) : type(type) {}

ensnare::ArrayType::ArrayType(Expr size, Type type) : size(size), type(type) {}

ensnare::FuncType::FuncType(Vec<Type> params, Opt<Type> return_type)
   : params(params), return_type(return_type) {}

ensnare::ConstType::ConstType(Type type) : type(type) {}
