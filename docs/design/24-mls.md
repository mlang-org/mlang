# 24. Language Server (`mls`)

`mls` implements the **Language Server Protocol** so every editor (VS Code,
Neovim, JetBrains, Emacs) gets the same first-class MLang experience from a single
codebase. Like `mfmt`, it is built on `libmlang_frontend`, so the editor's view of
the program is exactly the compiler's.

## 24.1 The "one frontend" payoff

The recurring theme of this design (Chapter 2) pays off most here: completion,
hover, go-to-definition, diagnostics, and rename are all computed by the *same*
name resolver and type checker the compiler uses. There is no separate "IDE
parser" to drift out of sync, which is the bug class that plagues language
tooling. When the compiler learns a new feature, `mls` understands it for free.

## 24.2 Incremental, error-tolerant analysis

An editor session is a long-lived process holding the project's analysis in
memory. On each keystroke `mls`:

- Re-lexes and re-parses only the edited file (the parser is error-tolerant,
  Chapter 5, so it produces a usable AST even mid-edit).
- Invalidates only the queries that depend on the change (the query system,
  Chapter 3), so type information for unaffected code is reused.
- Recomputes diagnostics for open files first (latency where the user is looking),
  then the rest in the background.

This is the same incremental machinery as batch compilation, tuned for latency
over throughput.

## 24.3 Supported LSP features

| Feature | Backed by |
|---------|-----------|
| Diagnostics (live) | the compiler's diagnostic engine (Chapter 8) |
| Completion | name resolution + type-directed candidate ranking |
| Hover | resolved type + doc comment (`///`) rendering |
| Go to definition / declaration | the resolved symbol table |
| Find references | a reverse index over the symbol table |
| Rename | reference index + validity check (no capture/conflict) |
| Signature help | overload set + current argument position |
| Document/workspace symbols | the declaration index |
| Semantic tokens (highlighting) | token kinds + resolved semantics |
| Code actions / quick fixes | the `Fix` payloads on diagnostics (Chapter 8) |
| Inlay hints | inferred types and parameter names |
| Formatting | `mfmt` via `libmlang_frontend` |
| Call hierarchy / type hierarchy | the symbol graph |

## 24.4 Semantic highlighting

Highlighting is **semantic**, not regex-based: `mls` classifies each token by what
it resolves to (a type vs a variable vs a function vs a parameter vs a mutable vs
an immutable binding), so the editor can color, for example, mutable variables
distinctly from immutable ones. Because this comes from real resolution, it is
correct even for tricky cases (shadowing, generics) that defeat textual
highlighters.

## 24.5 Code actions

Diagnostics carry machine-applicable fixes (Chapter 8): "add missing `match`
case", "make this `var`", "import `std.collections.List`", "remove unused
import", "handle this `Result`". `mls` surfaces them as quick fixes, and offers
refactors (extract function, inline variable, change signature) computed against
the typed AST so they are semantics-preserving.

## 24.6 Performance and architecture

- One process per workspace; analysis sharded by module and cached.
- Heavy work runs on a thread pool off the LSP message loop, so the editor never
  blocks.
- Memory is bounded by evicting cold files' arenas (Chapter 6) and recomputing on
  demand.
- A debounce coalesces rapid edits so analysis runs on settled text.

## 24.7 The VS Code extension

The repository ships *MLang Language Support* (`editors/vscode/`): a thin client
that launches `mls`, contributes the TextMate grammar for first-paint
highlighting (before semantic tokens arrive), wires the debugger (Chapter 25),
and registers commands (format, build, test). The same `mls` binary serves any
LSP editor; the VS Code extension is just the reference client.
