type
   NameKind* {.import_cpp: "clang::DeclarationName::NameKind".} = enum
      Identifier = 0
      ObjCZeroArgSelector = 1
      ObjCOneArgSelector = 2
      CXXConstructorName = 3
      CXXDestructorName = 4
      CXXConversionFunctionName = 5
      CXXOperatorName = 6
      CXXDeductionGuideName = 8
      CXXLiteralOperatorName = 9
      CXXUsingDirective = 10
      ObjCMultiArgSelector = 11
