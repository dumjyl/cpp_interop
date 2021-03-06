import ensnare/runtime
export runtime

type
   `blah-Foo`* {.import_cpp: "blah::Foo", header: "abc.hpp".} = object
      recursive_field: ptr `blah-Foo`
   `blah-VP`* = pointer
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
      xyz_field: `type_of(XYZ-xyz_field)`
   `type_of(anon_union_var)`* {.import_cpp: "decltype(anon_union_var)", header: "abc.hpp".} = object
   SepTypedef* {.import_cpp: "SepTypedef", header: "abc.hpp".} = object
   FnPtr* = proc (�0: CppInt)
   FnRef* = proc (�0: CppInt)
   FnRValueRef* = proc (�0: CppInt)

proc `{}`*(�: type[`blah-Foo`], a: CppInt, b: CppInt): `blah-Foo`
   {.import_cpp: "'0(@)", header: "abc.hpp".}
proc recusive_meth*(�: `blah-Foo`, x: ptr CppConst[`blah-Foo`])
   {.import_cpp: "#.recusive_meth(@)", header: "abc.hpp".}
proc calc*(�: `blah-Foo`, x: CppInt): CppInt
   {.import_cpp: "#.calc(@)", header: "abc.hpp".}
proc init*(�: type[`blah-Foo`], a: CppInt = 12): `blah-Foo`
   {.import_cpp: "'0::init(@)", header: "abc.hpp".}
proc `{}`*(�: type[`blah-Foo`], �1: var CppConst[`blah-Foo`]): `blah-Foo`
   {.import_cpp: "'0(@)", header: "abc.hpp".}
proc sum*(a: CppFloat, b: CppFloat): CppFloat
   {.import_cpp: "blah::sum(@)", header: "abc.hpp".}

var
   `blah-x`* {.import_cpp: "blah::x", header: "abc.hpp".}: `blah-Foo`
   anon_union_var* {.import_cpp: "anon_union_var", header: "abc.hpp".}: `type_of(anon_union_var)`
   ��your_identifiers_are��the���worst�* {.import_cpp: "__your_identifiers_are__the___worst_", header: "abc.hpp".}: CppInt
   �abc��a* {.import_cpp: "_abc__a", header: "abc.hpp".}: CppInt
   abc_a�* {.import_cpp: "abc_a_", header: "abc.hpp".}: CppInt
   abc_a���* {.import_cpp: "abc_a___", header: "abc.hpp".}: CppInt
   ��* {.import_cpp: "__", header: "abc.hpp".}: CppInt
   �* {.import_cpp: "_", header: "abc.hpp".}: CppInt

#% run

proc main =
   let x = `blah-Foo`{1, 2}
   assert(x.calc(3) == 6)
   assert(sum(1, 3) == 4)

main()
