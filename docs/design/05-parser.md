# 5. Parser

The parser turns tokens into an abstract syntax tree. It is a hand-written
**recursive-descent** parser for declarations and statements, with a **Pratt
(precedence-climbing)** sub-parser for expressions. The full formal grammar is in
[Chapter 34](34-grammar.md) and `docs/grammar/mlang.ebnf`.

## 5.1 Why hand-written

Generated parsers (yacc/bison/ANTLR) optimize for grammar authoring; hand-written
recursive descent optimizes for *error messages, recovery, and speed*, which is
what a production compiler actually needs. The grammar is LL(k) with small,
explicit lookahead, so recursive descent is a natural fit and the code reads like
the grammar.

## 5.2 Grammar class and ambiguity resolution

MLang's grammar is designed to be parseable with at most a few tokens of
lookahead. The genuinely ambiguous spots are handled deliberately:

- **`<` as generic-args vs less-than.** When parsing a postfix `<` after a name,
  the parser does a bounded *speculative* scan: it tries to read a type argument
  list ending in `>` followed by a token that can legally follow a generic
  instantiation (`(`, `::`, `{`, `.`). If the scan fails, `<` is the comparison
  operator. The speculation is bounded and rare, so it does not affect typical
  throughput.
- **Cast vs parenthesized expression.** `as` is an infix operator, so `(T)` is
  never a cast; casts are `expr as Type`. This removes the classic C ambiguity
  entirely.
- **Lambda vs parenthesized expression.** `(a, b) => ...` is decided by scanning
  for `=>` after the matching `)`. The parameter list grammar is a subset of the
  expression-list grammar, so the speculative parse is cheap.
- **Statement vs expression.** A block `{ ... }` in statement position is a block;
  in expression position it can be a struct literal or a lambda body, decided by
  context (the parser knows whether it is parsing an expression).

## 5.3 Expression parsing (Pratt)

Each token kind that can start or continue an expression has a binding power.
The expression parser is a single loop that consumes a prefix (`null` denotation)
then repeatedly consumes infix/postfix operators (`left` denotation) whose
binding power exceeds the current minimum. Precedence, highest to lowest:

| Level | Operators | Assoc |
|-------|-----------|-------|
| Postfix | `.` `?.` `()` `[]` `!` (non-null assert) | left |
| Unary prefix | `!` `-` `+` `~` `await` `new` | right |
| Power | `**` | right |
| Multiplicative | `*` `/` `%` | left |
| Additive | `+` `-` | left |
| Shift | `<<` `>>` | left |
| Relational | `<` `>` `<=` `>=` `is` `as` | left |
| Equality | `==` `!=` | left |
| Bitwise AND/XOR/OR | `&` `^` `\|` | left |
| Logical AND | `&&` | left |
| Logical OR | `\|\|` | left |
| Null-coalescing | `??` | right |
| Conditional | `?:` (ternary) | right |
| Assignment | `=` `+=` ... | right |
| Lambda | `=>` | right |

Encoding precedence as data (a table of binding powers) rather than as a tower of
recursive functions keeps the expression parser to one short, fast loop and makes
the precedence list auditable in one place.

## 5.4 Declarations and statements

Recursive-descent functions mirror the grammar: `parseNamespace`, `parseImport`,
`parseClass`, `parseStruct`, `parseEnum`, `parseInterface`, `parseExtension`,
`parseFn`, `parseField`, `parseStmt`, `parseBlock`, and so on. Modifiers
(`public`, `static`, `async`, `abstract`, `override`, `readonly`, ...) are parsed
as a leading set and validated for legality per declaration kind in semantic
analysis, not in the parser, so the error for "static constructor" is a good
semantic message rather than a parse failure.

## 5.5 Error recovery

The parser uses **panic-mode recovery with synchronization sets**. On an
unexpected token it:

1. Emits one diagnostic (it suppresses cascading errors until it has
   resynchronized, to avoid error storms).
2. Inserts an `Error` node so the AST is still well-formed and tooling can keep
   working.
3. Skips tokens until it reaches a member of the current context's *recovery
   set*: a statement starts at `let`/`var`/`if`/`for`/`return`/...; a declaration
   recovers at `fn`/`class`/`struct`/...; both recover at `}` and at a token that
   begins a line at the current indentation.

This is what allows `mls` to keep serving completion in a half-typed file and
allows one `mlangc` run to report every real error.

## 5.6 Trivia and full-fidelity mode

For `mfmt` and `mls`, the parser can run in **full-fidelity mode**, in which it
attaches leading/trailing trivia (whitespace, comments) to tokens and preserves
exact source ranges, so the AST can be printed back byte-for-byte. In normal
compilation this is off and trivia is discarded after attachment of doc comments.

## 5.7 Output

The parser produces an `AstModule`: a list of top-level declarations allocated in
a per-file bump arena (Chapter 6). Every node carries a `SourceRange`. The parser
performs no name binding and no type analysis; its only job is structure.
