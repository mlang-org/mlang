# 34. Grammar

The complete formal grammar of MLang is maintained as a single EBNF file at
[`docs/grammar/mlang.ebnf`](../grammar/mlang.ebnf). This chapter explains the
notation, the grammar's properties, and how it relates to the parser
(Chapter 5).

## 34.1 EBNF notation used

| Notation | Meaning |
|----------|---------|
| `=` | rule definition |
| `\|` | alternation |
| `( )` | grouping |
| `[ ]` | optional (zero or one) |
| `{ }` | repetition (zero or more) |
| `" "` | terminal (literal text) |
| `name` | non-terminal reference |
| `(* *)` | comment |

This is standard ISO/IEC 14977 EBNF with the conventional `[]`/`{}` extensions, so
the grammar is machine-checkable by off-the-shelf tools and unambiguous to read.

## 34.2 Grammar properties

- The grammar is **near-LL(k)**: it is parseable top-down with small, bounded
  lookahead. The few spots needing speculation (generic `<` vs less-than, lambda
  vs parenthesized expression) are documented in Chapter 5 and handled by bounded
  backtracking, not by a general GLR engine.
- Expressions are specified with an explicit **precedence and associativity
  table** (Chapter 5) rather than encoded as a deep rule tower, which keeps the
  grammar readable and the parser a single Pratt loop.
- Statement termination is **newline-sensitive** with an explicit continuation
  rule (Chapter 4), so the grammar notes where a newline acts as a terminator.

## 34.3 Lexical vs syntactic grammar

The EBNF file is in two parts:

1. **Lexical grammar**: how characters form tokens (identifiers, literals,
   operators, comments, string interpolation structure). This is what the lexer
   (Chapter 4) implements.
2. **Syntactic grammar**: how tokens form declarations, statements, and
   expressions. This is what the parser (Chapter 5) implements.

Keeping them separate mirrors the implementation and makes each independently
testable.

## 34.4 The grammar as a contract

The EBNF is normative: the hand-written parser must accept exactly the language
the grammar describes. This is enforced by a conformance corpus - a set of
programs known to be syntactically valid or invalid - that both the grammar
(via a generated reference parser used only in testing) and the production parser
are checked against. A divergence between the grammar and the parser is a bug in
one of them, caught in CI.

## 34.5 Stability

Grammar changes follow the RFC and edition process (Chapter 33): additive changes
are minor; changes that reject previously-valid syntax require a new edition. The
EBNF file is versioned alongside the language so any released compiler has a
matching, published grammar.

## 34.6 Reading order

To understand the language syntactically: read the lexical grammar first
(terminals), then `compilationUnit` and work down through `declaration`,
`statement`, and `expression`. The example programs (Chapter 35) exercise every
production and are a good companion while reading the grammar.
