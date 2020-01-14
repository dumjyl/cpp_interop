import
  pkg/std_ext,
  pkg/std_ext/mem_utils,
  pkg/std_ext/c_ffi/[str,
                     vec],
  clang_frontend/[pretty_printer,
                  ap_int,
                  common,
                  string_ref,
                  aps_int,
                  src_info,
                  twine,
                  iterator_range,
                  specific_decl_iterator]

const
  declH = "clang/AST/Decl.h"
  declTmplH = "clang/AST/DeclTemplate.h"
  declBaseH = "clang/AST/DeclBase.h"
  declNameH = "clang/AST/DeclarationName.h"
  typeH = "clang/AST/Type.h"
  exprH = "clang/AST/Expr.h"
  ctxH = "clang/AST/ASTContext.h"
  unitH = "clang/Frontend/ASTUnit.h"
  declCXXH = "clang/AST/DeclCXX.h"
  idTableH = "clang/Basic/IdentifierTable.h"

type
  CompilerInstance* {.import_cpp: "clang::CompilerInstance",
                      header: "clang/Frontend/CompilerInstance.h".} = object
  FrontendAction* {.import_cpp: "clang::FrontendAction",
                    header: "clang/Frontend/FrontendAction.h",
                    inheritable.} = object
  ASTFrontendAction* {.import_cpp: "clang::ASTFrontendAction",
                       header: "clang/Frontend/FrontendAction.h".} =
    object of FrontendAction
  ASTConsumer* {.import_cpp: "clang::ASTConsumer",
                 header: "clang/AST/ASTConsumer.h",
                 inheritable.} = object
  RecursiveASTVisitor*[T] {.import_cpp: "clang::RecursiveASTVisitor<'0>",
                            header: "clang/AST/RecursiveASTVisitor.h",
                            inheritable.} = object
  ASTUnit*{.importcpp: "clang::ASTUnit", header: unitH.} = object
  ASTContext*{.importcpp: "clang::ASTContext", header: ctxH.} = object

  DeclContext* {.import_cpp: "clang::DeclContext",
                 header: "clang/AST/DeclBase.h".} = object

  # --- Decls ---

  Decl*{.importcpp: "clang::Decl", header: declH.} = object of RootObj
  TranslationUnitDecl* {.import_cpp: "clang::TranslationUnitDecl",
                         header: "clang/AST/Decl.h".} = object of Decl
  NamedDecl*{.importcpp: "clang::NamedDecl", header: declH.} = object of Decl
  ValueDecl*{.importcpp: "clang::ValueDecl",
              header: declH.} = object of NamedDecl
  DeclaratorDecl*{.importcpp: "clang::DeclaratorDecl",
                   header: declH.} = object of ValueDecl
  TypeDecl*{.importcpp: "clang::TypeDecl", header: declH.} = object of NamedDecl
  TemplateDecl*{.importcpp: "clang::TemplateDecl",
                 header: declTmplH.} = object of NamedDecl
  RedeclarableTemplateDecl*{.importcpp: "clang::RedeclarableTemplateDecl",
                             header: declTmplH.} = object of TemplateDecl
  ClassTemplateDecl*{.importcpp: "clang::ClassTemplateDecl",
                      header: declTmplH.} = object of RedeclarableTemplateDecl
  FunctionTemplateDecl*{.importcpp: "clang::FunctionTemplateDecl",
                         header: declTmplH.} = object of RedeclarableTemplateDecl
  TagDecl*{.importcpp: "clang::TagDecl",
            header: declH.} = object of TypeDecl
  RecordDecl*{.importcpp: "clang::RecordDecl",
               header: declH.} = object of TagDecl
  EnumDecl*{.importcpp: "clang::EnumDecl", header: declH.} = object of TagDecl
  EnumConstantDecl*{.importcpp: "clang::EnumConstantDecl",
                     header: declH.} = object of ValueDecl
  CXXRecordDecl*{.importcpp: "clang::CXXRecordDecl",
                  header: declCXXH.} = object of RecordDecl
  FieldDecl*{.importcpp: "clang::FieldDecl",
              header: declH.} = object of DeclaratorDecl
  TypedefNameDecl*{.importcpp: "clang::TypedefNameDecl",
                    header: declH.} = object of TypeDecl
  VarDecl*{.importcpp: "clang::VarDecl",
            header: declH.} = object of DeclaratorDecl
  FunctionDecl*{.importcpp: "clang::FunctionDecl",
                 header: declH.} = object of DeclaratorDecl
  CXXMethodDecl*{.import_cpp: "clang::CXXMethodDecl",
                  header: declCxxH.} = object of FunctionDecl
  CXXConstructorDecl* {.import_cpp: "clang::CXXConstructorDecl",
                        header: declCxxH.} = object of CXXMethodDecl
  CXXConversionDecl* {.import_cpp: "clang::CXXConversionDecl",
                       header: declCxxH.} = object of CXXMethodDecl
  CXXDestructorDecl* {.import_cpp: "clang::CXXDestructorDecl",
                       header: declCxxH.} = object of CXXMethodDecl
  ParmVarDecl*{.importcpp: "clang::ParmVarDecl",
                header: declH.} = object of VarDecl
  NamespaceDecl*{.import_cpp: "clang::NamespaceDecl",
                  header: declH.} = object of NamedDecl
  CXXBaseSpecifier*{.import_cpp: "clang::CXXBaseSpecifier",
                     header: declCxxH.} = object
  UsingDecl* {.import_cpp: "clang::UsingDecl",
               header: declCxxH.} = object of NamedDecl
  TemplateTypeParmDecl* {.import_cpp: "clang::TemplateTypeParmDecl"
                          header: "clang/AST/DeclTemplate.h".} = object of TypeDecl

  # --- Decl Helpers ---

  DeclarationName*{.importcpp: "clang::DeclarationName",
                    header: declNameH.} = object
  IdentifierInfo*{.importcpp: "clang::IdentifierInfo",
                   header: idTableH.} = object
  TemplateParameterList*{.importcpp: "clang::TemplateParameterList",
                        header: declTmplH.} = object
  DeclIterator*[T]{.importcpp: "clang::DeclContext::specific_decl_iterator<'0>",
                    header: declBaseH.} = object

  # --- Types ---

  Type*{.importcpp: "clang::Type", header: typeH.} = object of RootObj
  QualType*{.importcpp: "clang::QualType", header: typeH.} = object
  BuiltinType*{.importcpp: "clang::BuiltinType",
                header: typeH.} = object of Type
  PointerType*{.importcpp: "clang::PointerType",
                header: typeH.} = object of Type
  ArrayType*{.importcpp: "clang::ArrayType", header: typeH.} = object of Type
  ConstantArrayType*{.importcpp: "clang::ConstantArrayType",
                      header: typeH.} = object of ArrayType
  IncompleteArrayType*{.importcpp: "clang::IncompleteArrayType",
                        header: typeH.} = object of ArrayType
  TypeWithKeyword*{.importcpp: "clang::TypeWithKeyword",
                    header: typeH.} = object of Type
  ElaboratedType*{.importcpp: "clang::ElaboratedType",
                   header: typeH.} = object of TypeWithKeyword
  TypedefType*{.importcpp: "clang::TypedefType",
                header: typeH.} = object of Type
  TagType*{.importcpp: "clang::TagType", header: typeH.} = object of Type
  RecordType*{.importcpp: "clang::RecordType",
               header: typeH.} = object of TagType
  EnumType*{.importcpp: "clang::EnumType", header: typeH.} = object of TagType
  ParenType*{.importcpp: "clang::ParenType", header: typeH.} = object of Type
  FunctionType*{.importcpp: "clang::FunctionType",
                 header: typeH.} = object of Type
  FunctionProtoType*{.importcpp: "clang::FunctionProtoType",
                      header: typeH.} = object of FunctionType
  FunctionNoProtoType*{.importcpp: "clang::FunctionNoProtoType",
                        header: typeH.} = object of FunctionType
  AdjustedType*{.importcpp: "clang::AdjustedType",
                 header: typeH.} = object of Type
  DecayedType*{.importcpp: "clang::DecayedType",
                header: typeH.} = object of AdjustedType
  ReferenceType*{.import_cpp: "clang::ReferenceType",
                  header: typeH.} = object of Type
  TemplateTypeParmType*{.import_cpp: "clang::TemplateTypeParmType",
                         header: typeH.} = object of Type
  InjectedClassNameType*{.import_cpp: "clang::InjectedClassNameType",
                          header: typeH.} = object of Type
  TemplateSpecializationType*{.import_cpp: "clang::TemplateSpecializationType",
                               header: typeH.} = object of Type
  TemplateName* {.import_cpp: "clang::TemplateName",
                  header: "clang/AST/TemplateName.h".} = object

  # --- Type Helpers ---

  ElaboratedTypeKeyword*{.size: sizeof(cuint).} = enum
    etkStruct
    etkInterface
    etkUnion
    etkClass
    etkEnum
    etkTypename
    etkNone
  CallingConv* = enum
    ccC
    ccX86StdCall
    ccX86FastCall
    ccX86ThisCall
    ccX86VectorCall
    ccX86Pascal
    ccWin64
    ccX86_64SysV
    ccX86RegCall
    ccAAPCS
    ccAAPCS_VFP
    ccIntelOclBicc
    ccSpirFunction
    ccOpenCLKernel
    ccSwift
    ccPreserveMost
    ccPreserveAll
    ccAArch64VectorCall

  # --- Exprs ---

  Expr*{.importcpp: "clang::Expr", header: exprH.} = object

proc get_ASTContext*(self: CompilerInstance): var ASTContext
  {.import_cpp: "getASTContext", header: "clang/Frontend/CompilerInstance.h".}

proc get_TranslationUnitDecl*(self: ASTContext): ptr TranslationUnitDecl
  {.import_cpp: "getTranslationUnitDecl" header: "clang/AST/ASTContext.h".}

# --- ASTUnit ---

proc getASTContext*(this: ASTUnit): ASTContext
  {.importcpp: "getASTContext", header: ctxH.}

# --- ASTContext ---

proc get_source_manager*(self: ASTContext): SourceManager
  {.importcpp: "getSourceManager", header: ctxH.}

# --- Type ---

proc dump*(this: Type)
  {.importcpp: "dump", header: typeH.}

proc getTypeClassName*(this: Type): cstring
  {.importcpp: "getTypeClassName", header: typeH.}

proc isBuiltinType*(this: Type): bool
  {.importcpp: "isBuiltinType", header: typeH.}

proc isBuiltin*(this: ptr Type): bool =
  result = isBuiltinType(this[])

proc isPointerType*(this: Type): bool
  {.importcpp: "isPointerType", header: typeH.}

proc isPointer*(this: ptr Type): bool =
  result = isPointerType(this[])

proc isArrayType*(this: Type): bool
  {.importcpp: "isArrayType", header: typeH.}

proc isArray*(this: ptr Type): bool =
  result = isArrayType(this[])

proc isFunctionType*(this: Type): bool
  {.importcpp: "isFunctionType", header: typeH.}

proc isFunction*(this: ptr Type): bool =
  result = isFunctionType(this[])

proc isFunctionNoProtoType*(this: Type): bool
  {.importcpp: "isFunctionNoProtoType", header: typeH.}

proc isFunctionNoProto*(this: ptr Type): bool =
  result = isFunctionNoProtoType(this[])

proc isFunctionProtoType*(this: Type): bool
  {.importcpp: "isFunctionProtoType", header: typeH.}

proc isFunctionProto*(this: ptr Type): bool =
  result = isFunctionProtoType(this[])

# --- QualType ---

proc get_type_ptr*(self: QualType): ptr Type
  {.import_cpp: "#.getTypePtr(@)", header: typeH.}

proc is_null*(self: QualType): bool
  {.import_cpp: "#.isNull(@)", header: typeH.}

# --- BuiltinType ---

proc isInteger*(this: BuiltinType): bool
  {.importcpp: "isInteger", header: typeH.}

proc isSignedInteger*(this: BuiltinType): bool
  {.importcpp: "isSignedInteger", header: typeH.}

proc isUnsignedInteger*(this: BuiltinType): bool
  {.importcpp: "isUnsignedInteger", header: typeH.}

proc isFloatingPoint*(this: BuiltinType): bool
  {.importcpp: "isFloatingPoint", header: typeH.}

proc isPlaceholderType*(this: BuiltinType): bool
  {.importcpp: "isPlaceholderType", header: typeH.}

proc getNameAsCString*(this: BuiltinType; policy: PrintingPolicy): cstring
  {.importcpp: "getNameAsCString", header: typeH.}

# --- PointerType ---

proc getPointeeType*(this: PointerType): QualType
  {.importcpp: "getPointeeType", header: typeH.}

# -- ArrayType ---

proc get_element_type*(this: ArrayType): QualType
  {.import_cpp: "getElementType", header: typeH.}

# --- ConstantArrayType ---

proc get_size*(this: ConstantArrayType): APInt
  {.import_cpp: "getSize", header: typeH.}

# --- TypeWithKeyword ---

proc getKeyword*(this: TypeWithKeyword): ElaboratedTypeKeyword
  {.importcpp: "getKeyword", header: typeH.}

# --- ElaboratedType ---

proc get_named_type*(self: ElaboratedType): QualType
  {.import_cpp: "getNamedType", header: typeH.}

proc getOwnedTagDecl*(this: ElaboratedType): TagDecl
  {.importcpp: "getOwnedTagDecl", header: typeH.}

# --- TypedefType ---

proc get_decl*(self: TypedefType): ptr TypedefNameDecl
  {.import_cpp: "#.getDecl(@)", header: typeH.}

proc desugar*(self: TypedefType): QualType
  {.importcpp: "desugar", header: typeH.}

# --- RecordType ---

proc get_decl*(self: RecordType): ptr RecordDecl
  {.import_cpp: "#.getDecl(@)", header: typeH.}

# --- EnumType ---

proc get_decl*(self: EnumType): ptr EnumDecl
  {.import_cpp: "#.getDecl(@)", header: typeH.}

# --- ParenType ---

proc getInnerType*(this: ParenType): QualType
  {.importcpp: "getInnerType", header: typeH.}

# --- FunctionType ---

proc get_return_type*(self: FunctionType): QualType
  {.importcpp: "getReturnType", header: typeH.}

proc get_return_type*(self: ptr FunctionType): ptr Type =
  result = get_return_type(deref self).get_type_ptr()

proc getCallConv*(this: FunctionType): CallingConv
  {.importcpp: "getCallConv", header: typeH.}

# --- FunctionProtoType ---

proc getNumParams*(this: FunctionProtoType): cuint
  {.importcpp: "getNumParams", header: typeH.}

proc getParamType*(this: FunctionProtoType, i: cuint): QualType
  {.importcpp: "getParamType", header: typeH.}

proc paramTyps*(this: ptr FunctionProtoType): seq[ptr Type] =
  for i in 0 ..< int(getNumParams(this[])):
    result.add(this[].getParamType(cuint(i)).getTypePtr())

proc isVariadic*(this: FunctionProtoType): bool
  {.importcpp: "isVariadic", header: typeH.}

# --- AdjustedType ---

proc getOriginalType*(this: AdjustedType): QualType
  {.importcpp: "getOriginalType", header: typeH.}

proc getAdjustedType*(this: AdjustedType): QualType
  {.importcpp: "getAdjustedType", header: typeH.}

# --- DecayedType ---

proc getDecayedType*(this: DecayedType): QualType
  {.importcpp: "getDecayedType", header: typeH.}

proc getPointeeType*(this: DecayedType): QualType
  {.importcpp: "getPointeeType", header: typeH.}

# --- DeclarationName ---

proc getAsString*(this: DeclarationName): cpp_string
  {.importcpp: "getAsString", header: declNameH.}

proc getAsIdentifierInfo*(this: DeclarationName): ptr IdentifierInfo
  {.importcpp: "getAsIdentifierInfo", header: declNameH.}

proc isIdentifier*(this: DeclarationName): bool
  {.importcpp: "isIdentifier", header: declNameH.}

proc dump*(this: DeclarationName)
  {.importcpp: "dump", header: declNameH.}

# --- IdentifierInfo ---

proc getName*(this: IdentifierInfo): StringRef
  {.importcpp: "getName", header: idTableH.}

proc getBuiltinID*(this: IdentifierInfo): c_uint
  {.importcpp: "getBuiltinID", header: idTableH.}

# --- Decl ---

proc getDeclKindName*(this: Decl): cstring
  {.importcpp: "getDeclKindName", header: declH.}

proc getCanonicalDecl*(this: Decl): ptr Decl
  {.importcpp: "getCanonicalDecl", header: declH.}

proc getNextDeclInContext*(this: Decl): ptr Decl
  {.importcpp: "getNextDeclInContext", header: declH.}

proc canon*(this: ptr Decl): ptr Decl =
  result = getCanonicalDecl(this[])

proc dumpColor*(self: Decl)
  {.importcpp: "dumpColor", header: declH.}

proc dump*(self: Decl)
  {.importcpp: "dump", header: declH.}

proc get_location*(this: Decl): SourceLocation
  {.import_cpp: "#.getLocation(@)", header: declH.}

# --- NamedDecl ---

proc get_name_as_string*(self: NamedDecl): cpp_string
  {.import_cpp: "#.getNameAsString(@)", header: declH.}

proc get_qualified_name_as_string*(self: NamedDecl): cpp_string
  {.import_cpp: "#.getQualifiedNameAsString(@)", header: declH.}

proc getDeclName*(this: NamedDecl): DeclarationName
  {.importcpp: "getDeclName", header: declH.}

# proc getName*(this: ptr NamedDecl): string =
#   let declName = getDeclName(this[])
#   let identInfo = declName.getAsIdentifierInfo()
#   if identInfo != nil:
#     result = $getName(identInfo[])
#   # echo isIdentifier(declName)
#   # dump(declName)
#   # echo cast[int](identInfo)
#   # identInfo
#   # echo isIdentifier
#     # result = $getName(identInfo[]).str().cStr()

#   # result = $cStr(getAsString(getDeclName(this[])))

proc name*(self: ptr NamedDecl): string =
  result = $self.deref.get_name_as_string()


proc qualified_name*(self: ptr NamedDecl): string =
  result = $self.deref.get_qualified_name_as_string()

# --- ValueDecl ---

proc getType*(this: ValueDecl): QualType
  {.importcpp: "getType", header: declH.}

# --- TypeDecl ---

proc getTypeForDecl*(this: TypeDecl): ptr Type
  {.importcpp: "getTypeForDecl", header: declH.}

# --- TemplateDecl ---

#proc getTemplateDecl*(c: Cursor): ptr TemplateDecl =
#  if c.kind != ckFunctionTemplate or c.data[cdkDecl] == nil or
#     not cast[ptr Decl](c.data[cdkDecl])[].isA(TemplateDecl):
#    raise newException(Defect, "cannot get TemplateDecl from cursor")
#  result = cast[ptr TemplateDecl](c.data[cdkDecl])

proc getTemplatedDecl*(this: TemplateDecl): ptr NamedDecl
  {.importcpp: "getTemplatedDecl", header: declTmplH.}

proc getTemplateParameters*(this: TemplateDecl): ptr TemplateParameterList
  {.importcpp: "getTemplateParameters", header: declTmplH.}

# --- TemplateParameterList ---

proc size*(self: TemplateParameterList): c_uint
  {.import_cpp: "size", header: declTmplH.}

proc get_param*(self: TemplateParameterList, idx: c_uint): ptr NamedDecl
  {.import_cpp: "getParam", header: declTmplH.}

iterator items*(self: ptr TemplateParameterList): ptr NamedDecl =
  for i in 0 ..< size(deref self).int:
    yield get_param(deref self, i.c_uint)

# --- specific_decl_iterator ---

proc inc*[T](this: DeclIterator[T])
  {.importcpp: "#.operator++(@)", header: declBaseH.}

proc `==`*[T](x, y: DeclIterator[T]): bool
  {.importcpp: "operator==(@)", header: declBaseH.}

proc cur*[T](this: DeclIterator[T]): ptr T
  {.importcpp: "#.operator*(@)", header: declBaseH.}

# --- TypedefNameDecl ---

# proc getCanonicalDecl*(this: TypedefNameDecl): ptr TypedefNameDecl
  # {.importcpp: "getCanonicalDecl", header: declH.}

proc getUnderlyingType*(this: TypedefNameDecl): QualType
  {.importcpp: "getUnderlyingType", header: declH.}

# --- TagDecl ---

proc isStruct*(this: TagDecl): bool
  {.importcpp: "isStruct", header: declH.}

proc isInterface*(this: TagDecl): bool
  {.importcpp: "isInterface", header: declH.}

proc isClass*(this: TagDecl): bool
  {.importcpp: "isClass", header: declH.}

proc isUnion*(this: TagDecl): bool
  {.importcpp: "isUnion", header: declH.}

proc isEnum*(this: TagDecl): bool
  {.importcpp: "isEnum", header: declH.}

# --- RecordDecl ---

proc field_begin*(self: RecordDecl): DeclIterator[FieldDecl]
  {.import_cpp: "field_begin", header: declBaseH.}

proc field_end*(self: RecordDecl): DeclIterator[FieldDecl]
  {.import_cpp: "field_end", header: declBaseH.}

proc field_empty*(self: RecordDecl): bool
  {.import_cpp: "field_empty", header: declBaseH.}

proc is_anon*(self: RecordDecl): bool
  {.import_cpp: "isAnonymousStructOrUnion", header: declH.}

iterator record_fields*(decl: ptr RecordDecl): ptr FieldDecl =
  var field_iter = field_begin(deref decl)
  while field_iter != field_end(deref decl):
    yield cur(field_iter)
    inc(field_iter)

# --- EnumDecl ---

proc enumFieldsBegin*(this: EnumDecl): DeclIterator[EnumConstantDecl]
  {.importcpp: "enumerator_begin", header: declH.}

proc enumFieldsEnd*(this: EnumDecl): DeclIterator[EnumConstantDecl]
  {.importcpp: "enumerator_end", header: declH.}

iterator enumFields*(this: ptr EnumDecl): ptr EnumConstantDecl =
  var fieldIter = enumFieldsBegin(this[])
  while fieldIter != enumFieldsEnd(this[]):
    yield cur(fieldIter)
    inc(fieldIter)

# --- EnumConstantDecl ---

proc getInitVal*(this: EnumConstantDecl): APSInt
  {.importcpp: "getInitVal", header: declH.}

proc enumVal*(this: ptr EnumConstantDecl): int64 =
  result = getInitVal(this[]).getExtValue()

# --- FieldDecl ---

proc isBitField*(this: FieldDecl): bool
  {.importcpp: "isBitField", header: declH.}

proc getBitWidthValue*(this: FieldDecl; astCtx: ASTContext): cuint
  {.importcpp: "getBitWidthValue", header: declH.}

# --- FunctionDecl ---

proc get_return_type*(self: FunctionDecl): QualType
  {.import_cpp: "getReturnType", header: declH.}

proc get_num_params*(self: FunctionDecl): cuint
  {.import_cpp: "getNumParams", header: declH.}

proc get_param_decl(self: FunctionDecl, i: cuint): ptr ParmVarDecl
  {.import_cpp: "getParamDecl", header: declH.}

proc get_return_type*(self: ptr FunctionDecl): ptr Type =
  result = get_return_type(deref self).get_type_ptr()

proc params*(self: ptr FunctionDecl): seq[ptr ParmVarDecl] =
  for i in 0 ..< int(self[].get_num_params()):
    result.add(self[].get_param_decl(c_uint(i)))

proc is_static*(self: FunctionDecl): bool
  {.import_cpp: "#.isStatic(@)".}

proc is_variadic*(self: FunctionDecl): bool
  {.import_cpp: "isVariadic", header: declH.}

proc getCallConv*(this: ptr FunctionDecl): CallingConv =
  var funcTyp = getType(this[]).getTypePtr
  assert(funcTyp.isA(FunctionProtoType))
  result = cast[ptr FunctionProtoType](funcTyp)[].getCallConv

type
  NameKind {.import_cpp: "clang::DeclarationName::NameKind",
             header: "clang/AST/DeclarationName.h".} = object
  DeclarationNameInfo* {.import_cpp: "clang::DeclarationNameInfo",
                         header: "clang/AST/DeclarationName.h".} = object

var
  nk_identifier* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::Identifier".}: NameKind
  nk_constructor* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXConstructorName".}: NameKind
  nk_destructor* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXDestructorName".}: NameKind
  nk_conversion* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXConversionFunctionName".}: NameKind
  nk_operator* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXOperatorName".}: NameKind
  nk_deduction_guide* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXDeductionGuideName".}: NameKind
  nk_literal_operator* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXLiteralOperatorName".}: NameKind
  nk_using_directive* {.no_decl, import_cpp: "clang::DeclarationName::NameKind::CXXUsingDirective".}: NameKind

proc `==`*(a, b: NameKind): bool {.import_cpp: "(# == #)".}

proc `$`*(k: NameKind): string =
  template impl(k_other) =
    if k == k_other:
      return ast_to_str(k_other)
  impl(nk_identifier)
  impl(nk_constructor)
  impl(nk_destructor)
  impl(nk_conversion)
  impl(nk_operator)
  impl(nk_deduction_guide)
  impl(nk_literal_operator)
  impl(nk_using_directive)
  return "some other NameKind shit"

proc get_name_info*(self: FunctionDecl): DeclarationNameInfo
  {.import_cpp: "#.getNameInfo(@)".}

proc get_name*(self: DeclarationNameInfo): DeclarationName
  {.import_cpp: "#.getName(@)".}

proc get_name_kind*(self: DeclarationName): NameKind
  {.import_cpp: "#.getNameKind(@)".}

# --- ParmVarDecl ---

# --- Expr ---

# --- ... ---

proc get_name*(self: NamedDecl): StringRef
  {.import_cpp: "#.getName(@)".}

proc get_decl_context*(self: Decl): ptr DeclContext
  {.import_cpp: "#.getDeclContext(@)".}

proc get_parent*(self: DeclContext): ptr DeclContext
  {.import_cpp: "#.getParent(@)".}

proc is_anonymous_namespace*(self: NamespaceDecl): bool
  {.import_cpp: "#.isAnonymousNamespace(@)".}

proc is_inline*(self: NamespaceDecl): bool
  {.import_cpp: "#.isInline(@)".}

export
  twine,
  string_ref

proc run_tool_on_code_with_args*(
    tool_action: ptr FrontendAction,
    code: Twine,
    args: cpp_vector[cpp_string],
    file_name: Twine,
    tool_name: Twine): bool
  {.import_cpp: "clang::tooling::runToolOnCodeWithArgs(@)",
    header: "clang/Tooling/Tooling.h".}

type
  OverloadedOperator* {.import_cpp: "clang::OverloadedOperatorKind",
                        header: "clang/Basic/OperatorKinds.h",
                        size: size_of(c_int).} = enum
    oo_None = ""
    oo_New = "new"
    oo_Delete = "delete"
    oo_Array_New = "new[]"
    oo_Array_Delete = "delete[]"
    oo_Plus = "+"
    oo_Minus = "-"
    oo_Star = "*"
    oo_Slash = "/"
    oo_Percent = "%"
    oo_Caret = "^"
    oo_Amp = "&"
    oo_Pipe = "|"
    oo_Tilde = "~"
    oo_Exclaim = "!"
    oo_Equal = "="
    oo_Less = "<"
    oo_Greater = ">"
    oo_PlusEqual = "+="
    oo_MinusEqual = "-="
    oo_StarEqual = "*="
    oo_SlashEqual = "/="
    oo_PercentEqual = "%="
    oo_CaretEqual = "^="
    oo_AmpEqual = "&="
    oo_PipeEqual = "|="
    oo_LessLess = "<<"
    oo_GreaterGreater = ">>"
    oo_LessLessEqual = "<<="
    oo_GreaterGreaterEqual = ">>="
    oo_EqualEqual = "=="
    oo_ExclaimEqual = "!="
    oo_LessEqual = "<="
    oo_GreaterEqual = ">="
    oo_Spaceship = "<=>"
    oo_AmpAmp = "&&"
    oo_PipePipe = "||"
    oo_PlusPlus = "++"
    oo_MinusMinus = "--"
    oo_Comma = ","
    oo_ArrowStar = "->*"
    oo_Arrow = "->"
    oo_Call = "()"
    oo_Subscript = "[]"

proc get_cxx_overloaded_operator*(self: DeclarationName): OverloadedOperator
  {.import_cpp: "#.getCXXOverloadedOperator(@)".}

type
  SourceInfo* = object
    line*: int
    col*: int
    file*: string

const
  srcMgrH = "clang/Basic/SourceManager.h"

proc get_file*(this: SourceManager; loc: SourceLocation): StringRef
  {.importcpp: "getFilename", header: srcMgrH.}

proc get_col*(this: SourceManager, loc: SourceLocation): c_uint
  {.importcpp: "getSpellingColumnNumber", header: srcMgrH.}

proc get_line*(this: SourceManager, loc: SourceLocation): c_uint
  {.importcpp: "getSpellingLineNumber", header: srcMgrH.}

template src_manager(ast_ctx: ptr ASTContext): SourceManager =
  ast_ctx[].get_source_manager()

proc src_info*(ast_ctx: ptr ASTContext, decl: ptr Decl): SourceInfo =
  let loc = decl[].get_location()
  result = SourceInfo(line: int ast_ctx.src_manager.get_line(loc),
                      col: int ast_ctx.src_manager.get_col(loc),
                      file: $ast_ctx.src_manager.get_file(loc))

proc methods*(self: CXXRecordDecl): iterator_range[specific_decl_iterator[CXXMethodDecl]]
  {.import_cpp: "#.methods(@)".}

proc bases*(self: CXXRecordDecl): iterator_range[ptr CXXBaseSpecifier]
  {.import_cpp: "#.bases(@)".}

iterator methods*(self: ptr CXXRecordDecl): ptr CXXMethodDecl =
  let methods = self.deref.methods
  var cur = methods.begin
  while cur != methods.`end`:
    yield cur.deref
    cur = succ(cur)

iterator bases*(self: ptr CXXRecordDecl): ptr CXXBaseSpecifier =
  let bases = self.deref.bases
  var cur = bases.begin
  while cur != bases.`end`:
    yield cur
    cur = cur.offset(1)

type AccessSpecifier* = enum as_public, as_protected, as_private, as_none

proc get_access*(self: Decl): AccessSpecifier
  {.import_cpp: "#.getAccess(@)".}

proc get_access*(self: CXXBaseSpecifier): AccessSpecifier
  {.import_cpp: "#.getAccessSpecifier(@)".}

type
  TypeClass* {.import_cpp: "clang::Type::TypeClass",
               header: "clang/AST/Type.h", pure.} = enum
    Builtin, Complex, Pointer, BlockPointer, LValueReference, RValueReference,
    MemberPointer, ConstantArray, IncompleteArray, VariableArray,
    DependentSizedArray, DependentSizedExtVector, DependentAddressSpace,
    Vector, DependentVector, ExtVector, FunctionProto, FunctionNoProto,
    UnresolvedUsing, Paren, Typedef, MacroQualified, Adjusted, Decayed,
    TypeOfExpr, TypeOf, Decltype, UnaryTransform, Record, Enum, Elaborated,
    Attributed, TemplateTypeParm, SubstTemplateTypeParm,
    SubstTemplateTypeParmPack, TemplateSpecialization, Auto,
    DeducedTemplateSpecialization, InjectedClassName, DependentName,
    DependentTemplateSpecialization, PackExpansion, ObjCTypeParam, ObjCObject,
    ObjCInterface, ObjCObjectPointer, Pipe, Atomic,

proc get_type_class*(self: Type): TypeClass
  {.import_cpp: "#.getTypeClass(@)".}

proc get_type_class*(self: ptr Type): TypeClass =
  result = get_type_class(deref self)

template derived_kind(TypeType: type[Type], first, last: TypeClass) =
  type `TypeType Class`* {.inject.} = range[TypeClass.first .. TypeClass.last]
  proc get_type_class*(self: ptr TypeType): `TypeType Class` =
    result = `TypeType Class`(self.cast_to(Type).get_type_class())

derived_kind(ArrayType, ConstantArray, DependentSizedArray)

proc get_pointee_type*(self: ReferenceType): QualType
  {.import_cpp: "#.getPointeeType(@)".}

type
  BuiltinKind* {.import_cpp: "clang::BuiltinType::Kind",
                 header: typeH.} = enum
    OCLImage1dRO, OCLImage1dArrayRO, OCLImage1dBufferRO, OCLImage2dRO,
    OCLImage2dArrayRO, OCLImage2dDepthRO, OCLImage2dArrayDepthRO,
    OCLImage2dMSAARO, OCLImage2dArrayMSAARO, OCLImage2dMSAADepthRO,
    OCLImage2dArrayMSAADepthRO, OCLImage3dRO, OCLImage1dWO,
    OCLImage1dArrayWO, OCLImage1dBufferWO, OCLImage2dWO, OCLImage2dArrayWO,
    OCLImage2dDepthWO, OCLImage2dArrayDepthWO, OCLImage2dMSAAWO,
    OCLImage2dArrayMSAAWO, OCLImage2dMSAADepthWO, OCLImage2dArrayMSAADepthWO,
    OCLImage3dWO, OCLImage1dRW, OCLImage1dArrayRW, OCLImage1dBufferRW,
    OCLImage2dRW, OCLImage2dArrayRW, OCLImage2dDepthRW, OCLImage2dArrayDepthRW,
    OCLImage2dMSAARW, OCLImage2dArrayMSAARW, OCLImage2dMSAADepthRW,
    OCLImage2dArrayMSAADepthRW, OCLImage3dRW, OCLIntelSubgroupAVCMcePayload,
    OCLIntelSubgroupAVCImePayload, OCLIntelSubgroupAVCRefPayload,
    OCLIntelSubgroupAVCSicPayload, OCLIntelSubgroupAVCMceResult,
    OCLIntelSubgroupAVCImeResult, OCLIntelSubgroupAVCRefResult,
    OCLIntelSubgroupAVCSicResult,
    OCLIntelSubgroupAVCImeResultSingleRefStreamout,
    OCLIntelSubgroupAVCImeResultDualRefStreamout,
    OCLIntelSubgroupAVCImeSingleRefStreamin,
    OCLIntelSubgroupAVCImeDualRefStreamin, Void, Bool, Char_U, UChar, WChar_U,
    Char8, Char16, Char32, UShort, UInt, ULong, ULongLong, UInt128, Char_S,
    SChar, WChar_S, Short, Int, Long, LongLong, Int128, ShortAccum, Accum,
    LongAccum, UShortAccum, UAccum, ULongAccum, ShortFract, Fract, LongFract,
    UShortFract, UFract, ULongFract, SatShortAccum, SatAccum, SatLongAccum,
    SatUShortAccum, SatUAccum, SatULongAccum, SatShortFract, SatFract,
    SatLongFract, SatUShortFract, SatUFract, SatULongFract, Half, Float,
    Double, LongDouble, Float16, Float128, NullPtr, ObjCId, ObjCClass, ObjCSel,
    OCLSampler, OCLEvent, OCLClkEvent, OCLQueue, OCLReserveID, Dependent,
    Overload, BoundMember, PseudoObject, UnknownAny, BuiltinFn,
    ARCUnbridgedCast, OMPArraySection,

proc get_kind*(self: BuiltinType): BuiltinKind
  {.import_cpp: "#.getKind(@)", header: typeH.}

proc get_kind*(self: ptr BuiltinType): BuiltinKind =
  result = get_kind(deref self)

proc get_identifier*(self: TemplateTypeParmType): ptr IdentifierInfo
  {.import_cpp: "#.getIdentifier(@)", header: typeH.}

proc get_injected_tst*(self: InjectedClassNameType): ptr TemplateSpecializationType
  {.import_cpp: "#.getInjectedTST(@)", header: typeH.}

proc get_num_args*(self: TemplateSpecializationType): c_uint
  {.import_cpp: "#.getNumArgs(@)", header: typeH.}

type TemplateArgument*{.import_cpp: "clang::TemplateArgument",
                        header: "clang/AST/TemplateBase.h".} = object

proc get_args*(self: TemplateSpecializationType): ptr TemplateArgument
  {.import_cpp: "#.getArgs(@)", header: typeH.}

proc args*(self: TemplateSpecializationType): seq[ptr TemplateArgument] =
  let args = self.get_args()
  for i in 0 ..< self.get_num_args():
    result.add(args.offset(i.int))

proc dump*(self: TemplateArgument)
  {.import_cpp: "#.dump(@)", header: "clang/AST/TemplateBase.h".}

proc get_parent*(self: CXXMethodDecl): ptr CXXRecordDecl
  {.import_cpp: "#.getParent(@)".}

proc get_template_name*(self: TemplateSpecializationType): TemplateName
  {.import_cpp: "#.getTemplateName(@)".}

proc get_as_template_decl*(self: TemplateName): ptr TemplateDecl
  {.import_cpp: "#.getAsTemplateDecl(@)", header: "clang/AST/TemplateName.h".}

proc is_null*(self: TemplateName): bool
  {.import_cpp: "#.isNull(@)".}

type FunctionTemplatedKind* {.import_cpp: "clang::FunctionDecl::TemplatedKind".} = enum
  ftkNonTemplate
  ftkFunctionTemplate
  ftkMemberSpecialization
  ftkFunctionTemplateSpecialization
  ftkDependentFunctionTemplateSpecialization

proc get_templated_kind*(self: FunctionDecl): FunctionTemplatedKind
  {.import_cpp: "#.getTemplatedKind(@)".}

proc get_described_function_template*(self: FunctionDecl): ptr FunctionTemplateDecl
  {.import_cpp: "#.getDescribedFunctionTemplate(@)".}

type
  DeclKind* {.pure, import_cpp: "clang::Decl::Kind", header: declH.} = enum
    AccessSpec, Block, Captured, ClassScopeFunctionSpecialization, Empty,
    Export, ExternCContext, FileScopeAsm, Friend, FriendTemplate, Import,
    LinkageSpec, Label, Namespace, NamespaceAlias, ObjCCompatibleAlias,
    ObjCCategory, ObjCCategoryImpl, ObjCImplementation, ObjCInterface,
    ObjCProtocol, ObjCMethod, bjCProperty, BuiltinTemplate, Concept,
    ClassTemplate, FunctionTemplate, TypeAliasTemplate, VarTemplate,
    TemplateTemplateParm, Enum, Record, CXXRecord, ClassTemplateSpecialization,
    ClassTemplatePartialSpecialization, TemplateTypeParm, ObjCTypeParam,
    TypeAlias, Typedef, UnresolvedUsingTypename, Using, UsingDirective,
    UsingPack, UsingShadow, ConstructorUsingShadow, Binding, Field,
    ObjCAtDefsField, ObjCIvar, Function, CXXDeductionGuide, CXXMethod,
    CXXConstructor, CXXConversion, CXXDestructor, MSProperty,
    NonTypeTemplateParm, Var, Decomposition, ImplicitParam, OMPCapturedExpr,
    ParmVar, VarTemplateSpecialization, VarTemplatePartialSpecialization,
    EnumConstant, IndirectField, OMPDeclareMapper, OMPDeclareReduction,
    UnresolvedUsingValue, OMPAllocate, OMPRequires, OMPThreadPrivate,
    ObjCPropertyImpl, PragmaComment, PragmaDetectMismatch, StaticAssert,
    TranslationUnit
proc get_kind*(self: Decl): DeclKind
  {.import_cpp: "#.getKind(@)", header: declBaseH.}

proc get_kind*(self: ptr Decl): DeclKind =
  result = get_kind(deref self)

template derived_kind(DeclType: type[Decl], first, last: DeclKind): untyped =
  type `DeclType Kind`* {.inject.} = range[DeclKind.first .. DeclKind.last]
  proc get_kind*(self: ptr DeclType): `DeclType Kind` =
    result = `DeclType Kind`(self.cast_to(Decl).get_kind())

derived_kind(NamedDecl, Label, UnresolvedUsingValue)
derived_kind(TypeDecl, DeclKind.Enum, UnresolvedUsingTypename)
derived_kind(TagDecl, DeclKind.Enum, ClassTemplatePartialSpecialization)
derived_kind(RecordDecl, DeclKind.Record, ClassTemplatePartialSpecialization)
derived_kind(CXXRecordDecl, CXXRecord, ClassTemplatePartialSpecialization)
derived_kind(TypedefNameDecl, DeclKind.ObjCTypeParam, DeclKind.Typedef)
#derived_kind(ClassTemplateSpecializationDecl,
#             ClassTemplateSpecialization,
#             ClassTemplatePartialSpecialization)

derived_kind(ValueDecl, Binding, UnresolvedUsingValue)
derived_kind(DeclaratorDecl, Field, VarTemplateSpecialization)
derived_kind(FunctionDecl, Function, CXXDestructor)
derived_kind(CXXMethodDecl, CXXMethod, CXXDestructor)

type
  TagKind* {.import_cpp: "clang::TagTypeKind".} = enum
    StructTag
    InterfaceTag
    UnionTag
    ClassTag
    EnumTag

proc get_tag_kind*(self: TagDecl): TagKind
  {.import_cpp: "#.getTagKind(@)".}
