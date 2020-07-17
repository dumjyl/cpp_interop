import ensnare/runtime

# type section

type
   `blah-Foo`* {.import_cpp: "blah::Foo".} = object

# function section

proc `{}`*(Self: type[`blah-Foo`], `a`: `cpp_int`, `b`: `cpp_int`): `blah-Foo`
   {.import_cpp: "blah::Foo::Foo(@)".}
proc `calc`*(self: `blah-Foo`, `x`: `cpp_int`): `cpp_int`
   {.import_cpp: "blah::Foo::calc(@)".}
