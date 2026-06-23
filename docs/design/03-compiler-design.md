# 3. Compiler Design

`mlangc` is a classic multi-pass compiler with a modern execution engine: it is
parallel, incremental, and cache-backed. This chapter covers the pipeline and
the cross-cutting machinery (threading, incrementality, caching). The individual
passes have their own chapters.

## 3.1 The pipeline

```
 .m sources
     │
     ▼
┌──────────┐   tokens    ┌──────────┐   AST    ┌────────────────┐
│  Lexer   │ ──────────► │  Parser  │ ───────► │ Name Resolution │
└──────────┘             └──────────┘          └────────┬───────┘
                                                        │ resolved AST
                                                        ▼
                                              ┌────────────────────┐
                                              │   Type Checking    │
                                              │ (inference, traits)│
                                              └─────────┬──────────┘
                                                        │ typed AST
                                                        ▼
                                       ┌────────────────────────────────┐
                                       │ Ownership & Borrow Analysis     │
                                       │ (move/borrow, ARC insertion)    │
                                       └─────────────┬──────────────────┘
                                                     │
                                                     ▼
                                          ┌────────────────────┐
                                          │  MIR lowering (SSA) │
                                          └─────────┬──────────┘
                                                    │ MIR
                                                    ▼
                                          ┌────────────────────┐
                                          │  Optimizer (passes) │
                                          └─────────┬──────────┘
                                                    │ optimized MIR
                                                    ▼
                                          ┌────────────────────┐
                                          │  LLVM code gen      │
                                          └─────────┬──────────┘
                                                    │ object + .mmi
                                                    ▼
                                                  linker
```

Each arrow is a typed, immutable data structure. A pass never mutates its input
in place; it produces a new representation. This makes the pipeline trivially
testable (feed input, assert on output) and safe to run across threads.

## 3.2 Phase ordering rationale

- **Name resolution before type checking.** Resolution binds every identifier to
  a declaration without knowing types; this breaks the chicken-and-egg problem
  of methods whose types depend on other methods. Resolution produces a fully
  qualified, scope-free AST.
- **Type checking before ownership analysis.** Move/borrow rules are expressed
  over types (a `Copy` value is never moved). Types must be known first.
- **Ownership analysis before MIR.** ARC retain/release and `drop` calls are
  inserted as part of lowering, so MIR is already explicit about lifetimes. The
  optimizer then deletes the ones it can prove redundant (Chapter 10, 13).
- **Optimize on MIR, not LLVM IR.** MLang-specific facts (nullability,
  ownership, exhaustiveness, generic shapes) are lost once we hand off to LLVM.
  The high-value, language-aware passes run on MIR while that information still
  exists; LLVM does the target-specific grind.

## 3.3 Parallel compilation

Parallelism happens at three granularities:

1. **Across modules.** The build is a DAG keyed on `.mmi` files. Independent
   modules compile on a thread pool; the only synchronization is the dependency
   edges. `mbuild` owns this scheduling and feeds `mlangc` one module at a time,
   or `mlangc` can be given the whole graph and schedule internally.
2. **Across functions within a module.** After type checking, every function
   body is an independent unit. MIR lowering, optimization, and LLVM codegen run
   per-function on a work-stealing pool. Function-level parallelism is where most
   of the wall-clock win comes from in large modules.
3. **Within parsing.** Lexing of a file is sequential, but files within a module
   are lexed and parsed in parallel; the parser is reentrant and allocates into a
   per-file arena, so there is no shared mutable state.

The shared structures that must be concurrent are: the **string interner**
(lock-striped, or a concurrent hash map), the **diagnostics sink** (lock-free
MPSC queue, drained and sorted by source position at the end so output is
deterministic), and the **type interner** (Chapter 7). Everything else is
thread-local.

> Determinism guarantee: regardless of thread count, the compiler produces
> byte-identical objects and diagnostics in a stable order. Non-determinism in a
> compiler is a category of bug we refuse to ship; all parallel reductions are
> order-independent or re-sorted.

## 3.4 Incremental compilation

Incrementality is built on two ideas: **query-based evaluation** and
**fingerprinting**.

- The compiler is internally structured as a memoized query system (in the
  spirit of `rustc`'s `salsa` or a build system). "What is the type of function
  `f`?" is a query whose result is cached and whose dependencies are recorded.
- Every input and intermediate gets a stable 128-bit fingerprint (a hash of its
  content, not its identity). When a file changes, only its fingerprint changes;
  queries whose recorded dependencies are unchanged return their cached result
  without recomputation.
- The key refinement is the **`.mmi` red/green distinction**: if you edit a
  function body but its *signature* and inline-eligibility are unchanged, the
  `.mmi` fingerprint is identical, so downstream modules are not rebuilt at all.
  Only the edited module's affected functions are re-lowered.

This is what makes the edit-compile loop feel instant in `mls`: a keystroke
re-runs the lexer and parser for one file and a handful of invalidated queries,
not the whole program.

## 3.5 Caching

There are two caches, with different lifetimes:

- **In-process query cache.** Lives for one `mlangc` invocation (or for the
  lifetime of the `mls` session). Holds parsed ASTs, resolved scopes, and
  computed types.
- **On-disk content-addressed cache.** Keyed by `(fingerprint of inputs, compiler
  version, target, flags)`. Stores compiled objects and `.mmi` files. A clean
  checkout that matches a previous build hits the cache for unchanged modules.
  The cache is safe to share across machines (it is content-addressed and
  hermetic), which enables a future distributed/remote cache.

Cache correctness rule: the cache key includes the compiler version and every
semantically relevant flag. A cache hit is *only* possible when the result is
guaranteed identical. We never trade correctness for a cache hit.

## 3.6 Error recovery and "always produce something"

The frontend never stops at the first error. The lexer emits error tokens and
resynchronizes; the parser inserts error nodes and recovers at statement and
declaration boundaries; the type checker assigns the `error` type (which is
compatible with everything, to avoid cascades) and continues. The result is that
one compile reports many real errors, and `mls` can still offer completion in a
file that does not yet parse. See Chapters 4, 5, and 8.

## 3.7 Compiler driver responsibilities

The `driver` layer (the thin top of `mlangc`) is responsible for: parsing
command-line arguments, building a `CompilerInvocation` (a fully resolved,
serializable description of the job), constructing the `TargetInfo`, wiring up
the thread pool and diagnostics, running the requested pipeline stage
(`--emit=tokens|ast|sema|ir|llvm|obj|exe`), and reporting timing. The driver is
the only layer allowed to touch `argc/argv`, the clock, and process exit codes.
