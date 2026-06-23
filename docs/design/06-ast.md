# 6. AST Structure

The AST is the central data structure of the frontend. Its design optimizes for
three things: fast allocation, cheap traversal, and stable source spans.

## 6.1 Allocation: arenas

All AST nodes for a file are allocated from a single bump-pointer **arena**. This
has three consequences:

- **Allocation is a pointer increment.** Parsing a large file does not thrash the
  general allocator.
- **Freeing is free.** When a file's AST is no longer needed (e.g. an `mls`
  session evicts it), the whole arena is released in one operation.
- **Nodes are referenced by pointer and never moved**, so child links are raw
  pointers (`Expr*`), not indices, which keeps traversal branch-light. Nodes are
  immutable after construction; later phases annotate them via *side tables* keyed
  by node id rather than by mutating the node.

This "parse into an arena, annotate via side tables" pattern is what makes the
same AST usable by the compiler (which adds types) and by `mfmt` (which adds
nothing) without either stepping on the other.

## 6.2 Node hierarchy

Nodes form three families, each with a common base carrying a `SourceRange` and a
`NodeId`:

```
Node
├── Decl          (declarations)
│   ├── NamespaceDecl   ImportDecl
│   ├── FnDecl          FieldDecl       ParamDecl
│   ├── ClassDecl       StructDecl      InterfaceDecl   EnumDecl
│   ├── ExtensionDecl   ConstructorDecl DestructorDecl
│   └── EnumCaseDecl    TypeParamDecl
├── Stmt          (statements)
│   ├── BlockStmt       LetStmt/VarStmt ExprStmt
│   ├── IfStmt          MatchStmt       ForStmt    WhileStmt   DoWhileStmt
│   ├── ReturnStmt      BreakStmt       ContinueStmt
│   └── TryStmt         ThrowStmt       YieldStmt
└── Expr          (expressions)
    ├── LiteralExpr     IdentExpr       ThisExpr   SuperExpr
    ├── UnaryExpr       BinaryExpr      AssignExpr TernaryExpr
    ├── CallExpr        IndexExpr       MemberExpr SafeMemberExpr
    ├── LambdaExpr      MatchExpr       IfExpr
    ├── NewExpr         CastExpr        IsExpr
    ├── StringInterp    ArrayLiteral    MapLiteral StructLiteral
    └── AwaitExpr       LaunchExpr      ErrorExpr
```

`TypeRef` is a parallel small hierarchy for *syntactic* types as written by the
user (`NamedType`, `NullableType`, `GenericType`, `FnType`, `TupleType`,
`ArrayType`). It is distinct from the *semantic* `Type` (Chapter 7): `TypeRef` is
what the parser produces; `Type` is what the type checker resolves it to.

## 6.3 Node representation in C++23

Nodes use a tagged hierarchy with a discriminant enum for fast `switch`-based
dispatch, plus a generated visitor for exhaustive traversal. A sketch:

```cpp
enum class NodeKind : uint8_t { /* every concrete node */ };

struct Node {
    NodeKind kind;
    SourceRange range;
    NodeId id;
};

struct Expr : Node { /* common expr data */ };

struct BinaryExpr final : Expr {
    BinaryOp op;
    Expr* lhs;
    Expr* rhs;
};
```

We deliberately avoid `std::variant` for the node hierarchy: a flat struct with a
`kind` tag plus arena allocation gives better cache behavior and smaller nodes
than a variant of every alternative, and the discriminant enables O(1) dispatch.
`std::variant` is used only for small, closed payloads inside a node (e.g. a
literal's value).

## 6.4 Visitors and traversal

Two traversal mechanisms are generated from the node list:

- A `ConstAstVisitor<R>` with one `visit<Node>()` method per kind, for analyses
  that compute a result.
- A `AstWalker` that does a pre/post-order depth-first walk and calls hooks, for
  passes that just need to see every node (e.g. the formatter, span collection).

Because traversal code is generated from a single declarative node list
(`compiler/include/mlang/ast/Nodes.def`), adding a node updates the visitor,
walker, and serializer at once and the compiler fails to build until every
`switch` handles it. Exhaustiveness is enforced by the C++ compiler.

## 6.5 Spans and trivia

Every node's `SourceRange` spans from the first token to the last token of the
construct, so a diagnostic can underline exactly the offending syntax. In
full-fidelity mode, leading/trailing trivia lists hang off tokens (not nodes),
which keeps the node small while still letting `mfmt` reproduce comments and
blank lines.

## 6.6 Serialization

The AST is serializable to a compact binary form for the `--emit=ast` dump and
for the `mls` cache. The serializer is generated from the same node list, so it
never drifts from the in-memory representation. Serialized ASTs are used for
golden tests (Chapter 26): a parser change that alters output is caught as a diff.

## 6.7 Why a separate `.mmi`, not a serialized AST

Downstream modules do not consume the AST; they consume the **module interface**
(`.mmi`), which holds *semantic* declarations (resolved types, generic
signatures, inline bodies) rather than syntax. Keeping syntax (AST) and interface
(`.mmi`) separate means a comment-only or formatting-only change never touches the
`.mmi` and never triggers a downstream rebuild (Chapter 3).
