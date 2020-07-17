import ensnare/runtime

# type section

type
   `blah-Foo`* {.import_cpp: "blah::Foo".} = object

# function section

proc `{}`*(Self: type[`blah-Foo`], a: cpp_int, b: cpp_int): `blah-Foo`
   {.import_cpp: "blah::Foo::Foo(@)", header: "abc.hpp".}
proc calc*(self: `blah-Foo`, a: cpp_int, b: cpp_int): `blah-Foo`
   {.import_cpp: "blah::Foo::calc(@)", header: "abc.hpp".}

# variable section

var
   `blah-x`* {.import_cpp: "blah::x".}: `blah-Foo`
