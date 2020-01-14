import
  pkg/std_ext,
  pkg/std_ext/c_ffi/str

const
  H = "llvm/ADT/StringRef.h"

type
  StringRef*{.importcpp: "llvm::StringRef", header: H.} = object

proc str*(self: StringRef): cpp_string
  {.importcpp: "#.str(@)", header: H.}

proc data*(self: StringRef): c_string
  {.importcpp: "#.data(@)", header: H.}

proc size*(self: StringRef): c_usize
  {.importcpp: "#.size(@)", header: H.}

proc `[]`*(self: StringRef, i: c_usize): char
  {.importcpp: "#.operator[](@)", header: H.}

proc `$`*(self: StringRef): string =
  result = string.init(self.size())
  for i in 0 ..< result.len:
    result[i] = self[c_usize(i)]
