import ensnare/runtime
export runtime

# --- types

type
   TypedefNameName* {.import_cpp: "TypedefNameName", header: "typedefs.hpp".} = object
   TypedefNameAlias* {.import_cpp: "TypedefNameAlias", header: "typedefs.hpp".} = object
   TypedefNameAliasAlias* = TypedefNameAlias
   TypedefAnonName* {.import_cpp: "TypedefAnonName", header: "typedefs.hpp".} = object
