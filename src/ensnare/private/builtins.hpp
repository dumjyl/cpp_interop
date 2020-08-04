#pragma once

#include "ensnare/private/type.hpp"

/// Contains constants to the low level atomic (in the AST sense) types for building more
/// complicated types.
namespace ensnare::builtins {
/// Initalize a builtin type.
Type init(const char* name) { return new_Type(new_Sym(name)); }

Type _schar = init("CppSChar");          ///< Represents `signed char`.
Type _short = init("CppShort");          ///< Represents `signed short`.
Type _int = init("CppInt");              ///< Represents `signed int`.
Type _long = init("CppLong");            ///< Represents `signed long`.
Type _long_long = init("CppLongLong");   ///< Represents `signed long long`.
Type _uchar = init("CppUChar");          ///< Represents `unsigned char`.
Type _ushort = init("CppUShort");        ///< Represents `unsigned short`.
Type _uint = init("CppUInt");            ///< Represents `unsigned int`.
Type _ulong = init("CppULong");          ///< Represents `unsigned long`.
Type _ulong_long = init("CppULongLong"); ///< Represents `unsigned long long`.
/// Represents `char`. Nim passes `funsigned-char` so this will always be unsigned for us.
Type _char = init("CppChar");
Type _wchar = init("CppWChar");              ///< Represents `whcar_t`.
Type _char8 = init("CppChar8");              ///< Represents `char8_t`.
Type _char16 = init("CppChar16");            ///< Represents `char16_t`.
Type _char32 = init("CppChar32");            ///< Represents `char32_t`.
Type _bool = init("CppBool");                ///< Represents `bool`.
Type _float = init("CppFloat");              ///< Represents `float`.
Type _double = init("CppDouble");            ///< Represents `double`.
Type _long_double = init("CppLongDouble");   ///< Represents `long double`.
Type _void = init("CppVoid");                ///< Represents `void`
Type _int128 = init("CppInt128");            ///< Represents `__int128_t`
Type _uint128 = init("CppUInt128");          ///< Represents `__uint128_t`
Type _neon_float16 = init("CppNeonFloat16"); ///< Represents `__fp16`
Type _ocl_float16 = init("CppOCLFloat16");   ///< Represents `half`
Type _float16 = init("CppFloat16");          ///< Represents `_Float16`
Type _float128 = init("CppFloat128");        ///< Represents `__float128`
// we treat cstddef as builtins too. But why?
Type _size = init("CppSize");          ///< Represents `std::size_t`
Type _ptrdiff = init("CppPtrDiff");    ///< Represents `std::ptrdiff_t`.
Type _max_align = init("CppMaxAlign"); ///< Represents `std::max_align_t`.
Type _byte = init("CppByte");          ///< Represents `std::byte`.
Type _nullptr = init("CppNullPtr");    ///< Represents `decltype(nullptr)` and `std::nullptr_t`.

Type _void_ptr = init("pointer");
} // namespace ensnare::builtins
