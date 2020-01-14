const
   cast_support = "llvm/Support/Casting.h"

proc is_a*[T0, T1](val: T0, _: typedesc[T1]): bool
   {.import_cpp: "llvm::isa<'2>(@)", header: cast_support.}

proc cast_to*[T0, T1](val: ptr T0, _: typedesc[T1]): ptr T1
   {.import_cpp: "llvm::cast<'2>(@)", header: cast_support.}

proc dyn_cast*[T0, T1](val: ptr T0, _: typedesc[T1]): ptr T1
   {.import_cpp: "llvm::dyn_cast<'2>(@)", header: cast_support.}

template try_as*[T0, T1](
      val: ptr T0,
      TryAsT: typedesc[T1],
      var_name: untyped
      ): bool {.deprecated.} =
   var var_name = val.dyn_cast(TryAsT)
   var_name != nil
