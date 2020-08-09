import ensnare/runtime
export runtime

type
   Vec* [size: static[CppSize]; T] {.import_cpp: "Vec<'0, '1>", header: "templ.hpp".} = object
      data: array[size, T]

proc foo_internal[T](a: T, b: T, �3: type[T]): T
   {.import_cpp: "foo<'3>(@)", header: "templ.hpp".}
proc foo*[T](a: T, b: T): T =
   foo_internal(a, b, T)
proc `{}`*[size: static[CppSize]; T](�: type[Vec[size, T]], val: T): Vec[size, T]
   {.import_cpp: "'0(@)", header: "templ.hpp".}
proc `{}`*[size: static[CppSize]; T; U](�: type[Vec[size, T]], val: T, nop_val: U): Vec[size, T]
   {.import_cpp: "'0(@)", header: "templ.hpp".}
proc nop_internal[size: static[CppSize]; T; U](�: Vec[size, T], val: Vec[size, U], �2: type[U])
   {.import_cpp: "#.nop<'3>(@)", header: "templ.hpp".}
proc nop*[size: static[CppSize]; T; U](�: Vec[size, T], val: Vec[size, U]) =
   nop_internal(�, val, U)
proc `[]`*[size: static[CppSize]; T](�: Vec[size, T], i: CppSize): var T
   {.import_cpp: "#.operator[](@)", header: "templ.hpp".}
proc double_size_internal[size: static[CppSize]; T; U](�: type[Vec[size, T]], val: U, �1: type[U]): CppInt
   {.import_cpp: "'0::double_size(@)", header: "templ.hpp".}
proc double_size*[size: static[CppSize]; T; U](�: type[Vec[size, T]], val: U): CppInt =
   double_size_internal(�, U)

#% run

proc `$`*[size: static[CppSize]; T](self: Vec[size, T]): string =
   result = "["
   for i in 0 ..< size:
      if result.len > 1:
         result &= ", "
      result &= $self[i.CppSize]
   result &= "]"

proc main =
   assert(foo(1, 3) == 4)
   let x = Vec[4.CppSize, int]{1}
   let y = Vec[4.CppSize, float]{2}
   x.nop(y)
   assert($x == "[1, 1, 1, 1]")
   assert($y == "[2.0, 2.0, 2.0, 2.0]")
   assert(x.we_need_a_method_template_test_too[:x.size, x.T, int]() == 1)
   assert(Vec[8.CppSize, uint8].double_size == 16)

main()
