import ensnare/runtime

# --- types

type
   `blah-Foo`* {.import_cpp: "blah::Foo", header: "abc.hpp".} = object
   SomeEnum* {.import_cpp: "SomeEnum", header: "abc.hpp".} = enum
      a = 0
      b = 1
      c = 2
   TypedefTest* {.import_cpp: "TypedefTest", header: "abc.hpp".} = object
   `TypedefTest-TypedefNamedNamed`* {.import_cpp: "TypedefTest::TypedefNamedNamed", header: "abc.hpp".} = object
   `TypedefTest-TypedefAnonNamed`* {.import_cpp: "TypedefTest::TypedefAnonNamed", header: "abc.hpp".} = object
   `TypedefTest-TypedefNamedAnon`* {.import_cpp: "TypedefTest::TypedefNamedAnon", header: "abc.hpp".} = object
   `type_of(XYZ-xyz_field)`* {.import_cpp: "decltype(XYZ::xyz_field)", header: "abc.hpp".} = object
   XYZ* {.import_cpp: "XYZ", header: "abc.hpp".} = object
      xyz_field: `type_of(XYZ-xyz_field`)
   `type_of(anon_union_var)`* {.import_cpp: "decltype(anon_union_var)", header: "abc.hpp".} = object

# --- routines

proc `{}`*(Self: type[`blah-Foo`], a: CppInt, b: CppInt): `blah-Foo`
   {.import_cpp: "blah::Foo::Foo(@)", header: "abc.hpp".}
proc calc*(self: `blah-Foo`, x: CppInt): CppInt
   {.import_cpp: "blah::Foo::calc(@)", header: "abc.hpp".}

# --- variables

var
   `blah-x`* {.import_cpp: "blah::x", header: "abc.hpp".}: `blah-Foo`
   anon_union_var* {.import_cpp: "anon_union_var", header: "abc.hpp".}: `type_of(anon_union_var)`
