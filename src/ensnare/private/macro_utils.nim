# FIXME: templates seem to mess up line info somehow?
# FIXME: fix the poor infix stmt list rendering

from std/strutils import split, join, indent
from std/macros import
   NimNodeKind, NimNodeKinds, kind, NimSymKind, sym_kind, get_impl, NimTypeKind, type_kind,
   get_type, get_type_inst, get_type_impl, `==`, `[]`, `[]=`, len, copy, insert, items, pairs,
   error, params, `params=`, body, `body=`, name, nnkCallKinds, line_info, line_info_obj,
   copy_line_info, tree_repr, get_ast, is_exported, bind_sym, str_val, parse_expr, parse_stmt,
   ident
{.push warnings: off.} # because we export things with a deprecated overload
export
   NimNodeKind, NimNodeKinds, kind, NimSymKind, sym_kind, get_impl, NimTypeKind, type_kind,
   get_type, get_type_inst, get_type_impl, `==`, `[]`, `[]=`, len, copy, insert, items, pairs,
   error, params, `params=`, body, `body=`, name, nnkCallKinds, line_info, line_info_obj,
   copy_line_info, tree_repr, get_ast, is_exported, bind_sym, str_val, parse_expr, parse_stmt,
   ident
{.pop.}

type
   NimTypeKinds* = set[NimTypeKind]
   NimSymKinds* = set[NimSymKind]
   NimNodeBuilder* = object
      kind: NimNodeKind
      line_info: NimNode

const
   routine_name_pos* = 0
   routine_pattern_pos* = 1
   routine_generic_params_pos* = 2
   routine_params_pos* = 3
   routine_pragmas_pos* = 4
   routine_reserved_pos* = 5
   routine_body_pos* = 6

   formals_return_type_pos* = 0

   object_def_pragmas_pos* = 0
   object_def_inherit_pos* = 1
   object_def_fields_pos* = 2

   call_like_name_pos* = 0

   postfix_star_pos* = 0
   postfix_ident_pos* = 1

   pragma_expr_expr_pos* = 0
   pragma_expr_pragma_pos* = 1

   ident_def_typ_pos* = ^2
   ident_def_val_pos* = ^1

const
   nnkAnySymChoice* = {nnkOpenSymChoice, nnkClosedSymChoice}
   nnkAnySym* = {nnkSym} + nnkAnySymChoice
   nnkAnyIdent* = {nnkIdent, nnkAccQuoted} + nnkAnySym

   nnkRoutines* = macros.RoutineNodes
   nnkStmtListLike* = {nnkStmtList, nnkStmtListExpr, nnkStmtListType}
   nnkBlockLike* = {nnkBlockStmt, nnkBlockExpr, nnkBlockType}
   nnkIfLike* = {nnkIfStmt, nnkIfExpr}
   nnkElifLike* = {nnkElifBranch, nnkElifExpr}
   nnkElseLike* = {nnkElse, nnkElseExpr}

# --- Error stuff

proc callsite*: NimNode =
   {.push warning[Deprecated]: off.}
   result = macros.callsite()
   {.pop.}

template is_init*(self: NimNode): bool = not self.is_nil

static:
   assert(not default(NimNode).is_init)
   assert(macros.new_NimNode(nnkNilLit).is_init)

func enclose*(str: string): string =
   ## Enclose some possibly multi-line string in a nice way
   # Remove any empty newlines at the start or end of `str`.
   let lines = str.split('\n')
   var result_lines = seq[string].default
   for i, line in lines:
      if line.len == 0 and (i == lines.low or i == lines.high):
         continue
      result_lines.add(line)
   const
      a = "⟨⟨⟨"
      b = "⟩⟩⟩"
   assert(result_lines.len != 0)
   result = new_string_of_cap(str.len + a.len + b.len)
   if result_lines.len > 1:
      result.add(a)
      result.add('\n')
      var first = true
      for line in result_lines:
         if not first:
            result.add('\n')
         result.add("  ")
         result.add(line)
         first = false
      result.add('\n')
      result.add(b)
   else:
      result.add("⟨")
      result.add(result_lines[0])
      result.add("⟩")

template msg(self, flair, contents) =
   self.add(ast_to_str(flair))
   self.add(": ")
   self.add(contents)
   self.add('\n')

template msg_node(self, node) =
   if node.is_init:
      when defined(verbose_macros1):
         msg self, Tree, node.tree_repr.enclose
      else:
         msg self, Note, "recompile with -d:verbose_macros1 for more information"
      msg self, Src, node.repr.enclose
   else:
      msg self, Warning, "node is not initialized"

var invocation {.compile_time.}: NimNode

template scope*(new_invocation, stmts) =
   let last_invocation = invocation
   invocation = new_invocation
   stmts
   defer: invocation = last_invocation

template fatal*(node_val: NimNode, msg_val: string) =
   var tmp = new_string_of_cap(256)
   if invocation.is_init:
      msg tmp, Invocation, invocation.repr.enclose
   msg_node tmp, node_val
   msg tmp, Message, msg_val
   tmp.set_len(tmp.len - 1)
   macros.error('\n' & indent(tmp, 2), node_val)

proc expect_string*(cond: string, message: string, node: NimNode): string =
   result = new_string_of_cap(256)
   msg_node result, node
   msg result, FailedExpectation, cond
   if message.len > 0:
      msg result, Message, message
   result.set_len(result.len - 1)
   result = '\n' & indent(result, 2)

template impl_expect*(cond, msg, val) =
   {.line.}:
      if not cond:
         macros.error(expect_string(ast_to_str(cond), msg, val), when val is NimNode: val
                                                                 else: val.detail)

func lit*[T](self: T): NimNode = macros.new_lit(self)

template `~=`*(self: NimNode, ident: string): bool = macros.eq_ident(self, ident)

proc `~=`*(self: NimNode, idents: openarray[string]): bool =
   for ident in idents:
      if self ~= ident:
         return true

template dump*(n: NimNode) =
   debug_echo("Dump ", ast_to_str(n).enclose, ":\n",
              indent("Tree: " & n.tree_repr.enclose & "\nStmt: " & n.repr.enclose, 2))

macro dump_ast*(stmts) = dump stmts

template expect*(n: NimNode, kind: NimNodeKind, user_note: string = ""): NimNode =
   {.line.}: impl_expect(n of kind, user_note, n)
   n

template expect*(n: NimNode, kinds: NimNodeKinds, user_note: string = ""): NimNode =
   {.line.}: impl_expect(n of kinds, user_note, n)
   n

template expect_min*(n: NimNode, min_len: int, user_note: string = ""): NimNode =
   {.line.}: impl_expect(n.len >= min_len, user_note, n)
   n

template expect*(n: NimNode, valid_len: int, user_note: string = ""): NimNode =
   {.line.}: impl_expect(n.len == valid_len, user_note, n)
   n

template expect*(n: NimNode, valid_len: Slice[int], user_note: string = ""): NimNode =
   {.line.}: impl_expect(n.len in valid_len, user_note, n)
   n

func low*(self: NimNode): int = 0

func high*(self: NimNode): int = self.len - 1

func add*(self: NimNode, sons: varargs[NimNode]) = discard macros.add(self, sons)

func delete*(self: NimNode, i: int, n = 1) = macros.del(self, i, n)

func set_len*(self: NimNode, len: int, fill = NimNode.default) =
   if len > self.len:
      for _ in 0 ..< len - self.len:
         self.add(copy(fill))
   else:
      self.delete(self.high, self.len - len)

proc line_info*(self: NimNode, info: NimNode): NimNode =
   # echo "BEFORE: ", self.line_info
   self.copy_line_info(info)
   # echo "AFTER: ", self.line_info
   result = self

template `@`*(kind_val: NimNodeKind, line_info_val: NimNode): NimNodeBuilder =
   NimNodeBuilder(kind: kind_val, line_info: line_info_val)

proc `{}`*(kind: NimNodeKind, sons: varargs[NimNode, `into{}`],): NimNode =
   result = macros.new_NimNode(kind)
   for son in sons:
      result.add(son)

proc `{}`*(self: NimNodeBuilder, sons: varargs[NimNode, `into{}`]): NimNode =
   result = macros.new_NimNode(self.kind, self.line_info)
   for son in sons:
      result.add(son)

template `into{}`*(self: NimNode): NimNode = self

template `into{}`*(self: string): NimNode = ident(self)

template exported*(self: NimNode): NimNode = nnkPostfix{"*".ident, self}

template pragmas*(self: NimNode, pragmas: openarray[NimNode]): NimNode =
   nnkPragmaExpr{self, nnkPragma{pragmas}}

func  `{}`*(kind: NimSymKind, name: string): NimNode = macros.gen_sym(kind, name)

template impl_kind_things(T, field) {.dirty.} =
   # template kind*(self: T): T = self
   # template kinds_of*(self: T): set[T] = {self}
   # template kinds_of*(self: set[T]): set[T] = self
   template `of`*(self: T, val: T): bool = self == val
   template `of`*(self: T, val: set[T]): bool = self in val
   template `of`*(self: NimNode, val: T): bool = self.field of val
   template `of`*(self: NimNode, val: set[T]): bool = self.field of val

template rewrite_ctrl_flow*[T: NimNode](self: T): bool = true

template subtype*(name, expr: NimNode, kinds: set[NimNodeKind]) = discard

impl_kind_things(NimNodeKind, kind)
impl_kind_things(NimTypeKind, type_kind)
impl_kind_things(NimSymKind, sym_kind)

func `$`*(self: NimNode): string = self.repr

func infix_join*(nodes: openarray[NimNode], op: string): NimNode =
   result = nodes[0]
   for i in 1 ..< nodes.len:
      result = macros.infix(result, op, nodes[i])

func is_command*(n: NimNode, name: string, valid_argument_range: Slice[int]): bool =
   result = n of nnkCommand and n.len - 1 in valid_argument_range and n[0] ~= name

func is_infix*(n: NimNode, name: string): bool =
   result = n of nnkInfix and n.len == 3 and n[0] ~= name

func is_prefix*(n: NimNode, name: string): bool =
   result = n of nnkPrefix and n.len == 2 and n[0] ~= name

proc replace*[T](
      self: NimNode,
      ctx: T,
      replace_fn: proc (self: NimNode, ctx: T): NimNode {.nim_call.}
      ): NimNode =
   result = replace_fn(self, ctx)
   if result == nil:
      result = self
      for i in 0 ..< self.len:
         self[i] = replace(self[i], ctx, replace_fn)

proc compound_ident(n: NimNode, sub_i = 0): string =
   result = ""
   for i in sub_i ..< n.len:
      result.add($n[i])

proc internal_quote(stmts: NimNode, dirty: bool): NimNode =
   type ReplaceCtx = ref object
      args: seq[NimNode]
      params: seq[NimNode]

   proc add(self: ReplaceCtx, expr: NimNode): NimNode =
      self.args.add(expr)
      self.params.add(ident(nskVar{":tmp"}.`$` & "_c8bd78kl46hqm9wpf0wnso8n0"))
      result = self.params[^1]

   proc replace_fn(self: NimNode, ctx: ReplaceCtx): NimNode =
      if self of nnkAccQuoted:

         # escaping injections
         # we concat into a string and check the first char instead of first
         # node in nnkAccQuoted to allow `![]` and `! []` 
         let str0 = self[0].str_val
         if str0[0] == '!': # check 
            if str0.len == 1:
               self.delete(0)
               if self.len == 0:
                  self.fatal("escaped identifier is empty")
            else:
               self[0] = self[0].str_val.substr(1).ident
               result = self[0]
         else:
            if self.len == 1: # expr injection fast path
               result = ctx.add(self[0])
            elif self[0] ~= "bind": # bind a symbol
               result = ctx.add(macros.new_call(bind_sym("bind_sym"),
                                                self.compound_ident(1).lit))
            else: # expr injection slow path. Some expression don't work though.
               let expr_str = self.compound_ident
               var expr = NimNode(nil)
               try:
                  expr = macros.parse_expr(expr_str)
               except ValueError:
                  self.fatal("failed to parse 'AST' expr")
               result = ctx.add(expr)

   let ctx = ReplaceCtx()
   let stmts = replace(stmts, ctx, replace_fn)
   let templ_sym = nskTemplate{"quote_payload"}
   let call = macros.new_call(templ_sym, ctx.args)
   let formals = nnkFormalParams{"untyped"}
   for param in ctx.params:
      formals.add(nnkIdentDefs{param, "untyped", nnkEmpty{}})
   let pragma = if dirty: nnkPragma{"dirty"} else: nnkEmpty{}
   let templ_def = nnkTemplateDef{templ_sym, nnkEmpty{}, nnkEmpty{}, formals, pragma, nnkEmpty{},
                                  stmts}
   result = nnkStmtList{templ_def, macros.new_call("get_ast".ident, call)}

macro AST*(stmts): auto =
   ## Similar to `macros.quote` but produces untyped AST with some extensions.
   ## Injection is done with backticks. There is no implicit injection. 
   ## Escape injection like `![]` to produce `[]`.
   ## Identifiers can be bound using `bind foo`.
   ## Some simple expressions can be used within backticks.
   ## 
   ## FIXME: line info gets overwritten
   internal_quote(stmts, true)

when false:
   macro AST_gensym*(stmts): auto = internal_quote(stmts, false)

macro `!`*(expr): NimNode =
   ## A sugar alias for `AST` for quick expression quoting. Strips a single `nnkPar` for things
   ## like `!(@[])`.
   let expr = if expr of nnkPar and expr.len == 1: expr[0] else: expr
   result = internal_quote(expr, true)

template add_AST*(self, stmts) =
   ## An appending alias for `AST`.
   ## Append `stmts` to `self` using `add`.
   self.add(AST(stmts))

func unsafe_subconv*(self: NimNode, _: set[NimNodeKind]): NimNode = self

func copy_line_info_rec*(self: NimNode, info: NimNode) =
   self.copy_line_info(info)
   for i in 0 ..< self.len:
      self[i].copy_line_info_rec(info)

proc `{}`*(Self: type[untyped], val: NimNode): NimNode =
   ## Remove any typing in the nodes.
   ##
   ## WIP
   result = val
   if result of nnkAnySym:
      if result.kind != nnkSym and result.len != 0:
         result = result[0]
      result = ident($result).line_info(result)
   else:
      for i in 0 ..< result.len:
         result[i] = untyped{result[i]}

proc is_command*(self: NimNode, name: string, len: int): bool =
   result = self of nnkCommand and self[0] ~= name and self.len - 1 == len

proc is_call*(self: NimNode, name: string, len: int): bool =
   result = self of nnkCall and self[0] ~= name and self.len - 1 == len

macro scope*(routine: untyped): auto =
   result = routine
   routine.body = nnkCall{["scope".ident, nnkCall{["callsite".ident]}, routine.body]}

template line_info_dir*(self: NimNode): string = self.line_info_obj.file_name.split_file.dir

iterator as_ident_defs*(self: NimNode, offset: int): NimNode =
   ## Reinterpret something that isn't a container for nnkIdentDefs as if it was, starting at `offset`.
   if self.expect({nnkObjConstr, nnkCall, nnkFormalParams, nnkBracketExpr}).len - offset > 0:
      var untyped_idents_buf = seq[NimNode].default
      for i in offset ..< self.len:
         let arg = self[i]
         case arg.kind:
         of nnkInfix:
            if arg[0] ~= ":=":
               yield nnkIdentDefs{arg[1], nnkEmpty{}, arg[2]}
            else:
               arg.fatal("invalid parameters")
         of nnkAnyIdent - nnkAnySymChoice:
            untyped_idents_buf.add(arg)
         of nnkExprColonExpr:
            var result = nnkIdentDefs{untyped_idents_buf}
            untyped_idents_buf.set_len(0)
            result.add(arg[0])
            if arg[1].is_infix(":="):
               result.add(arg[1][1])
               result.add(arg[1][2])
            else:
               result.add(arg[1])
               result.add(nnkEmpty{})
            yield result
         else: arg.fatal("unexpected param")
      if untyped_idents_buf.len != 0:
         for untyped_ident in untyped_idents_buf:
            yield nnkIdentDefs{untyped_ident, nnkEmpty{}, nnkEmpty{}}

proc collect*(kind: NimNodeKind, vals: NimNode): NimNode =
   result = kind{}
   for val in vals:
      result.add(val)
