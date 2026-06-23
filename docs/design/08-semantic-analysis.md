# 8. Semantic Analysis

Semantic analysis is everything between "we have a typed-correctly-shaped tree"
and "we can lower to IR". It runs in well-defined sub-phases, each producing a
new annotation layer over the AST.

## 8.1 Name resolution

Resolution binds every `IdentExpr`, `TypeRef`, and member access to the
declaration it refers to. It runs in two passes:

1. **Collection.** Walk every module and record every top-level and nested
   declaration into a hierarchical `ScopeTree` (namespace → file → type →
   function → block). This pass is order-independent, so forward references
   ("use a function declared later in the file") just work, and it can run per
   file in parallel.
2. **Binding.** Walk expression and type positions and resolve each name against
   the scope chain plus imports. Overload sets are collected here but not
   resolved (overload resolution needs types, so it happens in type checking).

Resolution handles: imports (`import std.io.Console`, including renaming and
wildcard rules), shadowing (a `let` may shadow an outer binding; the compiler
warns on accidental shadowing), `this`/`super`, extension methods (Chapter 18),
and visibility (`public`/`internal`/`protected`/`private`). Visibility violations
are reported here with a note pointing at the declaration's modifier.

## 8.2 Type checking

Type checking (Chapter 7) assigns a `Type*` to every expression, resolves
overloads and generic instantiations, checks assignability, verifies interface
conformance, and runs exhaustiveness checking for `match`. It is bidirectional:
information flows down (the expected type of an expression) and up (the inferred
type of a subexpression). Errors assign the `error` type and continue, so one
run reports many independent errors.

### Exhaustiveness

`match` over a sealed type (an enum, or a `sealed` class hierarchy) must cover
every case or the compiler rejects it. The checker computes the set of
uncovered patterns and reports them explicitly ("missing case: `Rectangle`"),
which doubles as refactoring support: add a variant, and every non-exhaustive
match becomes a compile error pointing you at the code to update.

### Definite assignment

A `let`/`var` must be assigned on every path before use. A `readonly`/`let`
field must be assigned exactly once, in the constructor, on every path. The
analysis is a forward dataflow over the control-flow graph; using a
possibly-unassigned variable is an error, not a runtime null.

## 8.3 Ownership and borrow analysis

This is the phase that delivers memory safety without a tracing GC. It runs on a
control-flow graph derived from the typed AST and enforces an affine discipline
on owning values.

### Core rules

- Every value has exactly one **owner** at a time. Assigning or passing an owning
  value **moves** it; the source binding is then dead and using it is an error
  ("use after move", with a note at the move site).
- `Copy` types (small `AnyVal`: integers, floats, `bool`, `char`, and structs
  explicitly marked `Copy`) are copied instead of moved, so the source stays
  valid. `Copy`-ness is part of the type.
- A value can be **borrowed** instead of moved: a shared borrow (`&T`, many
  readers) or an exclusive borrow (`&mut T`, one writer). The standard
  aliasing-XOR-mutability rule holds: any number of shared borrows, or exactly
  one exclusive borrow, never both. This is what statically rules out data races
  (Chapter 15) and iterator invalidation.
- When an owner goes out of scope, its `destructor` (Chapter, RAII) runs and its
  storage is reclaimed. The compiler inserts these `drop` calls; the programmer
  never writes `free`.

### The ergonomics escape valve: ARC

Pure ownership (Rust-style) is powerful but has a learning curve, especially for
shared, long-lived, or cyclic object graphs. MLang's distinguishing choice is
that *reference types fall back to automatic reference counting* when the
ownership analysis cannot statically prove a unique owner:

- If a class instance is shared in a way ownership cannot track (stored in two
  places, captured by an escaping closure), it becomes a reference-counted
  `Rc<T>` automatically. The compiler inserts `retain`/`release`.
- The optimizer then **elides** the counts it can prove are unnecessary
  (Chapter 10, 13), so the common case pays nothing and only genuinely shared
  objects pay for counting.

The result is a gradient: write simple code and get ARC's ergonomics; add borrows
where you want zero-overhead and the compiler rewards you. Either way the four
classes of memory bug are impossible. The full model is [Chapter 13](13-memory-management.md).

## 8.4 Effect and async checking

The checker verifies async discipline: `await` is legal only inside an `async`
function or coroutine; an `async fn` returns `Task<T>`; calling an `async fn`
without `await` yields a `Task<T>` value (it does not run yet). It also checks
that `yield` appears only in a generator and that `throw`/`?` are used in
functions whose signature admits errors (Chapter 17).

## 8.5 Const evaluation

A `const` context (array sizes `[T; N]`, enum discriminants, `const fn` calls,
static initializers) is evaluated at compile time by a small, sandboxed
interpreter over MIR. Const-eval has no I/O and a bounded step budget, so it
cannot hang the compiler. It also powers `constant folding` upstream of the
optimizer (Chapter 10).

## 8.6 Diagnostics quality

Every semantic error carries: a stable code (e.g. `E0382: use of moved value`), a
primary span, secondary spans (the move site, the prior borrow, the declaration),
and where possible a `Fix` (an applicable edit, surfaced by `mls` as a code
action). The catalog of diagnostics is part of the spec and is regression-tested,
so error messages are a stable, documented interface, not an accident.

## 8.7 Output of semantic analysis

The phase produces: a fully typed AST (every node has a `Type*`), a resolved
symbol table, per-function CFGs with ownership facts, and the inserted ARC/drop
operations recorded as annotations. This is the input to MIR lowering
(Chapter 9).
