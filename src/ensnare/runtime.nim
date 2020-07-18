# c++ standard: https://timsong-cpp.github.io/cppwp/n4861/
#               this is c++20ish, but we target c++17

{.pass_c: "-std=c++17".}

from std/os import is_absolute, split_file, `/`
import private/macro_utils

const hpp = "ensnare/private/runtime.hpp"

macro cpp_expr*(T, pattern, args): auto =
   let id = nskProc{"cpp_expr"}
   let formals = nnkFormalParams{T}
   let call = nnkCall{id}
   if args.len == 0:
      formals.add(nnkIdentDefs{nskParam{"cpp_arg"}, "auto".ident, nnkEmpty{}})
      call.add(args)
   else:
      for arg in args:
         formals.add(nnkIdentDefs{nskParam{"cpp_arg"}, "auto".ident, nnkEmpty{}})
         call.add(arg)
   let def = nnkProcDef{id, nnkEmpty{}, nnkEmpty{}, formals,
                        nnkPragma{nnkExprColonExpr{"import_cpp".ident, pattern}},
                        nnkEmpty{}, nnkEmpty{}}
   result = nnkStmtList{def, call}

type LaunderClassBuf*[T] {.import_cpp: "ensnare::runtime::LaunderClassBuf<'0>",
                           header: hpp.} = object

proc unsafe_destroy[T](self: var LaunderClassBuf[T])
   {.import_cpp: "#.unsafe_destroy(@)".}

proc unsafe_move[T](this: var LaunderClassBuf[T], other: LaunderClassBuf[T])
   {.import_cpp: "#.unsafe_move(@)".}

proc unsafe_copy[T](this: LaunderClassBuf[T]): LaunderClassBuf[T]
   {.import_cpp: "#.unsafe_copy(@)".}

proc unsafe_deref[T](this: LaunderClassBuf[T]): lent T
   {.import_cpp: "#.unsafe_deref(@)".}

type cpp*[T] = object
   detail {.export_c.}: LaunderClassBuf[T]

template unsafe_copy*[T](this: cpp[T]): cpp[T] =
   ## An implimentation would expose this with `copy` if it is a copyable type.
   cpp[T](detail: unsafe_copy(this.detail))

proc `=destroy`[T](self: var cpp[T]) =
   unsafe_destroy(self.detail)

proc `=`*[T](dst: var cpp[T], src: cpp[T]) {.error: "use `copy` instead; if available.".}

proc `=sink`*[T](dst: var cpp[T], src: cpp[T]) =
   unsafe_move(dst.detail, src.detail)

template `{}`*[T](Self: type[cpp[T]], args: varargs[untyped]): cpp[T] =
   cpp[T](detail: cpp_expr(LaunderClassBuf[T], "'0(@)", args))

template deref*[T](self: cpp[T]): lent T = unsafe_deref(self.detail)

const cstddef_h = "<cstddef>"

# These cover the `basic.fundamental` types.

type
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#1
   CppSChar* {.import_cpp: "signed char".} = int8
   CppShort* {.import_cpp: "short int".} = int16
   CppInt* {.import_cpp: "int".} = int32
   CppLong* {.import_cpp: "long int".} = int64
   CppLongLong* {.import_cpp: "long long int".} = int64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#2
   CppUChar* {.import_cpp: "unsigned char".} = uint8
   CppUShort* {.import_cpp: "unsigned short int".} = uint16
   CppUInt* {.import_cpp: "unsigned int".} = uint32
   CppULong* {.import_cpp: "unsigned long int".} = uint64
   CppULongLong* {.import_cpp: "unsigned long long int".} = uint64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#7
   CppChar* = char

# https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#8
when defined(windows): # FIXME: when is this signed and unsigned
                       #        source for windows vs other sizing.
   type CppWChar* {.import_cpp: "wchar_t".} = uint16
else:
   type CppWChar* {.import_cpp: "wchar_t".} = uint32

type
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#9
   CppChar8* {.import_cpp: "char8_t".} = char
   CppChar16* {.import_cpp: "char8_t".} = uint16
   CppChar32* {.import_cpp: "char8_t".} = uint32
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#10
   CppBool* = bool
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#12
   CppFloat* {.import_cpp: "float".} = float32
   CppDouble* {.import_cpp: "double".} = float64
   CppLongDouble* {.import_cpp: "long double".} = float64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#13
   CppVoid* {.import_cpp: "decltype(void)".} = ptr object
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#14, `std::nullptr_t` comes with cstddef.

type # exntensions
   CppInt128* {.import_cpp: "__int128_t".} = int64
   CppUInt128* {.import_cpp: "__uint128_t".} = uint64
   CppNeonFloat16* {.import_cpp: "__fp16".} = uint16
   CppOCLFloat16* {.import_cpp: "half".} = uint16
   CppFloat16* {.import_cpp: "_Float16".} = uint16
   CppFloat128* {.import_cpp: "__float128".} = float64

type
   CppConst*[T] {.import_cpp: "ensnare::runtime::Constant<'0>", header: hpp.} = object
   CppUnsizedArray*[T] {.import_cpp: "ensnare::runtime::UnsizedArray<'0>", header: hpp.} = object
   CppCharPtr* = ptr CppConst[char]

proc `=`*[T](dst: var CppUnsizedArray[T], src: CppUnsizedArray[T]) {.error.}
proc `=sink`*[T](dst: var CppUnsizedArray[T], src: CppUnsizedArray[T]) {.error.}

type # <cstddef> types
   CppSize* {.import_cpp: "std::size_t", header: cstddef_h.} = uint
   CppPtrDiff* {.import_cpp: "std::ptrdiff_t", header: cstddef_h.} = int
   # this one is weird. We can maybe determine manually ourselves.
   # cpp_max_align_t* {.import_cpp: "std::max_align_t", header: cstddef_h.} =
   CppByte* {.import_cpp: "std::byte", header: cstddef_h.} = object
   CppNullPtr* {.import_cpp: "std::nullptr_t", header: cstddef_h.} = type_of(nil)

proc cpp_static_assert*(condition: bool, message: CppCharPtr) {.import_cpp: "static_assert(@)".}

proc cpp_size_of*[T](_: T): CppSize {.import_cpp: "sizeof(@)".}
proc cpp_size_of*[T](_: type[T]): CppSize {.import_cpp: "sizeof('0)".}

var cpp_standard {.no_decl, import_cpp: "__cplusplus".}: CppLong

template c_str(str: untyped{nkStrLit}): CppCharPtr =
   cast[CppCharPtr](str.c_string)

cpp_static_assert(cpp_standard >= 201703.CppLong, "c++17 or later is required".c_str)
#cpp_static_assert(cpp_size_of(cpp_int) == 4, "Only LP64 systems are supported")
#cpp_static_assert(cpp_size_of(cpp_long) == 8, "Only LP64 systems are supported")

proc split_namespace(self: NimNode): seq[NimNode] =
   for ident in self:
      if not (ident ~= "-"):
         result.add(ident)

type CppSrc* = object
   dir: string
   name: string
   ext: string

template `{}`*(Self: type[CppSrc], src: string): Self =
   var (dir, name, ext) = split_file(src)
   #if not is_absolute(dir):
   #   dir = split_file(instantiation_info(-1, true).filename).dir
   CppSrc(dir: dir, name: name, ext: ext)

proc import_name(ns_parts: openarray[NimNode]): string =
   var first = true
   for ns_part in ns_parts:
      if not first:
         result &= "::"
      result = result & $ns_part
      first = false
   result &= "(@)"

macro cpp_load*(src: static[CppSrc], decl: untyped): auto =
   result = decl
   let ns_parts = split_namespace(decl[0])
   decl[0] = ns_parts[^1]
   let import_name = import_name(ns_parts)
   if decl[4] of nnkEmpty:
      decl[4] = !({.import_cpp: `import_name`.})
   else: assert(false, "FIXME")
   case src.ext:
   of ".hpp":
      decl[4].add(nnkExprColonExpr{"header", lit(src.dir/src.name&src.ext)})
   else: decl.fatal("unsupported extension")

template cpp_forward_compiler*(arg: static[string]) =
   {.pass_c: arg.}

template cpp_forward_linker*(arg: static[string]) =
   {.pass_l: arg.}

template cpp_include_dir*(dir: static[string]) =
   cpp_forward_compiler("-I" & dir)

template cpp_link_lib*(library: static[string]) =
   cpp_forward_linker("-l" & library)

macro cpp_link_lib*(libraries: static[openarray[string]]) =
   result = nnkStmtList{}
   for library in libraries:
      result.add(!`bind cpp_link_lib`(`library.lit`))

template cpp_compile_src*(src: static[string]) =
   {.compile: src.}

template emit_cpp*(src: static[string]) =
   {.emit: src.}
