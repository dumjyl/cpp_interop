# c++ standard: https://timsong-cpp.github.io/cppwp/n4861/
#               this is c++20ish, but we target c++17

{.pass_c: "-std=c++17".}

from std/os import is_absolute, split_file, `/`
import private/macro_utils

const rt_hpp = "rt.hpp"

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

type LaunderClassBuf*[T] {.import_cpp: "ensnare::rt::LaunderClassBuf<'0>",
                           header: rt_hpp.} = object


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
   cpp_schar* {.import_cpp: "signed char".} = int8
   cpp_short* {.import_cpp: "short int".} = int16
   cpp_int* {.import_cpp: "int".} = int32
   cpp_long* {.import_cpp: "long int".} = int64
   cpp_long_long* {.import_cpp: "long long int".} = int64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#2
   cpp_uchar* {.import_cpp: "unsigned char".} = uint8
   cpp_ushort* {.import_cpp: "unsigned short int".} = uint16
   cpp_uint* {.import_cpp: "unsigned int".} = uint32
   cpp_ulong* {.import_cpp: "unsigned long int".} = uint64
   cpp_ulong_long* {.import_cpp: "unsigned long long int".} = uint64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#7
   cpp_char* = char

# https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#8
when defined(windows): # FIXME: when is this signed and unsigned
                       #        source for windows vs other sizing.
   type cpp_wchar_t* {.import_cpp: "wchar_t".} = uint16
else:
   type cpp_wchar_t* {.import_cpp: "wchar_t".} = uint32

type
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#9
   cpp_char8_t* {.import_cpp: "char8_t".} = char
   cpp_char16_t* {.import_cpp: "char8_t".} = uint16
   cpp_char32_t* {.import_cpp: "char8_t".} = uint32
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#10
   cpp_bool* = bool
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#12
   cpp_float* {.import_cpp: "float".} = float32
   cpp_double* {.import_cpp: "double".} = float64
   cpp_long_double* {.import_cpp: "long double".} = float64
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#13
   cpp_void* {.import_cpp: "decltype(void)".} = ptr object
   # https://timsong-cpp.github.io/cppwp/n4861/basic.fundamental#14, `std::nullptr_t` comes with cstddef.

type # exntensions
   cpp_int128* {.import_cpp: "__int128_t".} = int64
   cpp_uint128* {.import_cpp: "__uint128_t".} = uint64
   cpp_neon_float16* {.import_cpp: "__fp16".} = uint16
   cpp_ocl_float16* {.import_cpp: "half".} = uint16
   cpp_float16* {.import_cpp: "_Float16".} = uint16
   cpp_float128* {.import_cpp: "__float128".} = float64

type
   cpp_const*[T] {.import_cpp: "ensnare::rt::constant<'0>".} = object
   cpp_unsized_array*[T] {.import_cpp: "ensnare::rt::unsized_array<'0>", header: rt_hpp.} = object
   cpp_char_ptr* = ptr cpp_const[char]
   cpp_mutable_char_ptr* = ptr char

proc `=`*[T](dst: var cpp_unsized_array[T], src: cpp_unsized_array[T]) {.error.}
proc `=sink`*[T](dst: var cpp_unsized_array[T], src: cpp_unsized_array[T]) {.error.}

type # <cstddef> types
   cpp_size_t* {.import_cpp: "std::size_t", header: cstddef_h.} = uint
   cpp_ptrdiff_t* {.import_cpp: "std::ptrdiff_t", header: cstddef_h.} = int
   # this one is weird. We can maybe determine manually ourselves.
   # cpp_max_align_t* {.import_cpp: "std::max_align_t", header: cstddef_h.} =
   cpp_byte* {.import_cpp: "std::byte", header: cstddef_h.} = object
   cpp_nullptr_t* {.import_cpp: "std::nullptr_t", header: cstddef_h.} = type_of(nil)

proc cpp_static_assert*(condition: bool, message: c_string) {.import_cpp: "static_assert(@)".}

proc cpp_size_of*[T](_: T): cpp_size_t {.import_cpp: "sizeof(@)".}
proc cpp_size_of*[T](_: type[T]): cpp_size_t {.import_cpp: "sizeof('0)".}

var cpp_standard {.no_decl, import_cpp: "__cplusplus".}: cpp_long

cpp_static_assert(cpp_standard >= 201703.cpp_long, "c++17 or later is required")
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
   if not is_absolute(dir):
      dir = split_file(instantiation_info(-1, true).filename).dir
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

template emit_cpp*(src: static[string]) =
   {.emit: src.}
