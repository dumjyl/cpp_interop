#pragma once

#include "ensnare/private/ir.hpp"

/// Contains constants to the low level atomic (in the AST sense) types for building more
/// complicated types.
namespace ensnare::builtins {
/// Initalize a builtin type.
Node<Type> init(const char* name) { return node<Type>(node<Sym>(name)); }

/// An atomic Type.
using Builtin = const Node<Type>;

Builtin _schar = init("CppSChar");          ///< Represents `signed char`.
Builtin _short = init("CppShort");          ///< Represents `signed short`.
Builtin _int = init("CppInt");              ///< Represents `signed int`.
Builtin _long = init("CppLong");            ///< Represents `signed long`.
Builtin _long_long = init("CppLongLong");   ///< Represents `signed long long`.
Builtin _uchar = init("CppUChar");          ///< Represents `unsigned char`.
Builtin _ushort = init("CppUShort");        ///< Represents `unsigned short`.
Builtin _uint = init("CppUInt");            ///< Represents `unsigned int`.
Builtin _ulong = init("CppULong");          ///< Represents `unsigned long`.
Builtin _ulong_long = init("CppULongLong"); ///< Represents `unsigned long long`.
/// Represents `char`. Nim passes `funsigned-char` so this will always be unsigned for us.
Builtin _char = init("CppChar");
Builtin _wchar = init("CppWChar");              ///< Represents `whcar_t`.
Builtin _char8 = init("CppChar8");              ///< Represents `char8_t`.
Builtin _char16 = init("CppChar16");            ///< Represents `char16_t`.
Builtin _char32 = init("CppChar32");            ///< Represents `char32_t`.
Builtin _bool = init("CppBool");                ///< Represents `bool`.
Builtin _float = init("CppFloat");              ///< Represents `float`.
Builtin _double = init("CppDouble");            ///< Represents `double`.
Builtin _long_double = init("CppLongDouble");   ///< Represents `long double`.
Builtin _void = init("CppVoid");                ///< Represents `void`
Builtin _int128 = init("CppInt128");            ///< Represents `__int128_t`
Builtin _uint128 = init("CppUInt128");          ///< Represents `__uint128_t`
Builtin _neon_float16 = init("CppNeonFloat16"); ///< Represents `__fp16`
Builtin _ocl_float16 = init("CppOCLFloat16");   ///< Represents `half`
Builtin _float16 = init("CppFloat16");          ///< Represents `_Float16`
Builtin _float128 = init("CppFloat128");        ///< Represents `__float128`
// we treat cstddef as builtins too. But why?
Builtin _size = init("CppSize");          ///< Represents `std::size_t`
Builtin _ptrdiff = init("CppPtrDiff");    ///< Represents `std::ptrdiff_t`.
Builtin _max_align = init("CppMaxAlign"); ///< Represents `std::max_align_t`.
Builtin _byte = init("CppByte");          ///< Represents `std::byte`.
Builtin _nullptr = init("CppNullPtr");    ///< Represents `decltype(nullptr)` and `std::nullptr_t`.

Builtin _void_ptr = init("pointer");
} // namespace ensnare::builtins
