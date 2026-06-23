# 37. Development Phases

This chapter is the delivery plan: the milestones that take MLang from an empty
repository to a stable 1.0, and what "done" means at each. It complements the
long-term [roadmap](33-roadmap.md), which looks beyond 1.0.

## 37.1 Phase 0 - Foundations (this repository)

**Goal:** the project skeleton and a compiling frontend.

- Repository structure, build system, CI, contribution docs. (done)
- Complete design document and EBNF grammar. (done)
- Compiler frontend that builds on a stock C++23 toolchain:
  `SourceManager`, `DiagnosticEngine`, `Lexer`, the AST, and a `Parser`, driven by
  `mlangc --emit=tokens|ast`. (done)
- Backend, runtime, stdlib, and tools scaffolded behind clean interfaces. (done)

**Exit criteria:** `cmake --build` succeeds; `mlangc` tokenizes and parses the
example programs; the design is complete enough to implement against.

## 37.2 Phase 1 - MVP (a language that runs)

**Goal:** compile and run a non-trivial single-module program.

- Name resolution and a core type checker (primitives, functions, classes,
  structs, `T?`, basic generics).
- MIR construction and a minimal pass set (`mem2reg`, constant folding, DCE,
  inlining).
- LLVM backend producing native executables for `x86_64-linux`.
- Ownership/ARC for the common cases; deterministic `destructor`s.
- A minimal runtime (allocator, panic, startup) and a `core` + `io` +
  `collections` slice of the standard library.
- `mbuild build`/`run` for a single package.

**Exit criteria:** the `hello`, `basics`, `types`, and `errors` examples compile
to native binaries and produce correct output; the test framework runs.

## 37.3 Phase 2 - Alpha (the language is complete)

**Goal:** the full language surface, single platform.

- Full type system: variance, `where` clauses, exhaustive `match`, flow typing,
  selective erasure.
- Full ownership/borrow checker with the four guarantees (Chapter 13).
- Coroutines, the scheduler, channels, structured concurrency (Chapters 15, 16).
- Exceptions/`Result`/`?`, zero-cost unwinding (Chapter 17).
- Reflection + attributes (Chapter 19); most of the standard library.
- `mango` resolving and fetching dependencies; `mfmt`; `mls` with diagnostics,
  completion, hover, go-to-definition.
- Incremental + parallel compilation with the on-disk cache (Chapter 3).

**Exit criteria:** the `concurrency` and `webserver` examples run; a real project
(the standard library itself) builds with `mbuild` and `mango`; `mls` is usable
daily in VS Code.

## 37.4 Phase 3 - Beta (cross-platform, hardened, fast)

**Goal:** production readiness on the tier-1 platforms.

- Tier-1 targets green in CI: Linux/macOS/Windows on x86-64 and AArch64
  (Chapter 11).
- Performance work to hit the benchmark targets (Chapter 38): escape analysis,
  ARC elision, devirtualization, bounds-check elimination, LTO/PGO.
- Debugger (DWARF/CodeView + DAP), full `mls` refactorings, coverage.
- Security hardening, sanitizer + fuzzing CI, supply-chain features in `mango`
  (Chapter 31).
- API stabilization pass on the standard library; deprecation tooling.

**Exit criteria:** benchmark targets met within tolerance; no known memory-safety
holes in the safe subset; the toolchain is self-hosting in the sense that the
stdlib and tools build reproducibly across all tier-1 platforms.

## 37.5 Phase 4 - 1.0 (stable)

**Goal:** a stability commitment.

- The language and standard-library API are frozen under the edition mechanism
  (Chapter 33); source compatibility within the 1.x line.
- The design document graduates to a versioned normative specification
  (Chapter 34).
- Documented release cadence, RFC process, and security policy.
- Published binaries, an installer, and a documentation site.

**Exit criteria:** an external team can build and ship a production service in
MLang relying on documented stability guarantees.

## 37.6 Working method throughout

- **Design doc is the source of truth**; code that diverges updates the doc or is
  fixed.
- **Tested invariants**: the MIR verifier, differential testing, `mfmt`
  idempotence, parser conformance, and benchmark regression gates run in CI from
  Phase 0.
- **Conventional commits** and small, reviewed PRs; the changelog
  (`CHANGELOG.md`) tracks every notable change.
- **Vertical slices**: prefer making one feature work end-to-end (lex to native)
  over building each layer fully before the next, so the pipeline is exercised
  early and integration risk is paid down continuously.
