import
  ./string_ref

const
  srcLocH = "clang/Basic/SourceLocation.h"
  srcMgrH = "clang/Basic/SourceManager.h"

type
  SourceManager*{.importcpp: "clang::SourceManager", header: srcMgrH.} = object
  FileID*{.importcpp: "clang::FileID", header: srcLocH.} = object
  SourceLocation*{.importcpp: "clang::SourceLocation",
                   header: srcLocH.} = object
  LocInfo = object
    lineNum: int
    colNum: int
    path: string

proc dump*(this: SourceLocation; sm: SourceManager)
  {.importcpp: "dump", header: srcLocH.}

proc getFilename*(this: SourceManager; loc: SourceLocation): StringRef
  {.importcpp: "getFilename", header: srcMgrH.}

proc getSpellingColumnNumber*(this: SourceManager, loc: SourceLocation): c_uint
  {.importcpp: "getSpellingColumnNumber", header: srcMgrH.}

proc getSpellingLineNumber*(this: SourceManager, loc: SourceLocation): c_uint
  {.importcpp: "getSpellingLineNumber", header: srcMgrH.}

template filename*(loc: SourceLocation): string =
  $getSourceManager(astCtx(ctx)).getFilename(loc)

template getLocInfo*(loc: SourceLocation): LocInfo =
  template srcMgr: SourceManager = getSourceManager(astCtx(ctx))
  let tmpLoc = loc
  LocInfo(lineNum: int(srcMgr.getSpellingLineNumber(tmpLoc)),
          colNum: int(srcMgr.getSpellingColumnNumber(tmpLoc)),
          path: $srcMgr.getFilename(tmpLoc))
