import ensnare/runtime

# --- types

type
   `blah-Foo`* {.import_cpp: "blah::Foo", header: "abc.hpp".} = object
   SomeEnum* {.import_cpp: "SomeEnum", header: "abc.hpp".} = enum
      a = 0
      b = 1
      c = 2

# --- routines

proc `{}`*(Self: type[`blah-Foo`], a: CppInt, b: CppInt): `blah-Foo`
   {.import_cpp: "blah::Foo::Foo(@)", header: "abc.hpp".}
proc calc*(self: `blah-Foo`, x: CppInt): CppInt
   {.import_cpp: "blah::Foo::calc(@)", header: "abc.hpp".}

# --- variables

var
   `blah-x`* {.import_cpp: "blah::x", header: "abc.hpp".}: `blah-Foo`
