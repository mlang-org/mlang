# 2. Architecture

This chapter describes the system as a whole: the components, the artifacts they
exchange, and the boundaries between them. Later chapters zoom into each box.

## 2.1 The toolchain at a glance

```
                         ┌──────────────────────────────────────────┐
   developer  ────────►  │                 mbuild                    │
   editor (mls)          │  (orchestrates: resolves, compiles, links)│
                         └───────┬───────────────────────┬──────────┘
                                 │                        │
                          calls mango               invokes mlangc
                                 │                        │
                    ┌────────────▼──────────┐   ┌─────────▼───────────────────┐
                    │        mango          │   │           mlangc            │
                    │  resolve / fetch /    │   │  lex → parse → sema → IR →   │
                    │  build dep graph      │   │  optimize → codegen (LLVM)   │
                    └────────────┬──────────┘   └─────────┬───────────────────┘
                                 │                        │
                          packages/ cache         object files + metadata
                                 │                        │
                                 └───────────┬────────────┘
                                             │
                                       system linker (lld)
                                             │
                                    ┌────────▼─────────┐
                                    │   executable /   │  links against
                                    │   shared library │  mlang-runtime
                                    └──────────────────┘
```

The arrows are deliberately one-directional. `mbuild` knows about `mango` and
`mlangc`; neither of those knows about `mbuild`. The compiler is a pure function
from sources plus a build plan to artifacts; everything stateful (caches,
registries, lockfiles) lives in the tools around it.

## 2.2 Components and responsibilities

| Component | Binary | Language | Responsibility |
|-----------|--------|----------|----------------|
| Compiler | `mlangc` | C++23 | Translate `.m` sources to native objects + metadata. |
| Runtime | `mlang-runtime` | C++23 + a little asm | Allocator, scheduler, intrinsics, unwinder, startup. |
| Standard library | (`.m`) | MLang | Everything that can be written in MLang (Chapter 20). |
| Package manager | `mango` | C++23 | Manifests, version resolution, registry, lockfiles. |
| Build tool | `mbuild` | C++23 | Build graph, target orchestration, incremental driver. |
| Formatter | `mfmt` | C++23 (shares the parser) | Canonical pretty-printing. |
| Language server | `mls` | C++23 (shares the frontend) | LSP: completion, hover, rename, diagnostics. |
| Debugger bridge | (part of `mls`/`mbuild`) | C++23 | DWARF emission config + DAP adapter. |

## 2.3 Why a shared compiler library

`mlangc`, `mfmt`, and `mls` are thin executables over one static library,
`libmlang_frontend`. The lexer, parser, AST, name resolver, and type checker are
written once and consumed three ways:

- `mlangc` runs the full pipeline.
- `mfmt` stops after parsing and pretty-prints the AST + trivia.
- `mls` runs an *incremental, error-tolerant* version of the pipeline and keeps
  the result in memory across edits.

This is the single most important architectural decision in the project: there
is exactly one parser and one type checker in the entire ecosystem. An editor
can never disagree with the compiler about whether a program is valid, because
they are the same code. (This is the mistake that costs other languages years of
"the IDE says it's fine but the build fails" bug reports.)

## 2.4 Artifacts and intermediate formats

| Artifact | Extension | Producer | Consumer |
|----------|-----------|----------|----------|
| Source | `.m` | human | lexer |
| Module interface | `.mmi` | `mlangc` | `mlangc` (downstream modules), `mls` |
| Serialized IR | `.mir` | `mlangc -emit=ir` | optimizer, tooling |
| Object file | `.o`/`.obj` | `mlangc` backend | linker |
| Compilation cache entry | (content-addressed) | `mlangc` | `mlangc` (incremental) |
| Lockfile | `mango.lock` | `mango` | `mango`, `mbuild` |
| Manifest | `mango.json` | human | `mango`, `mbuild` |

The **module interface file** (`.mmi`) is the linchpin of separate
compilation. When `mlangc` compiles a module it emits a compact, versioned
binary description of everything another module can see: public declarations,
their types, generic signatures, inline-eligible bodies, and a content hash.
Downstream modules type-check against the `.mmi`, never against the source. This
is what makes the build a proper DAG and incremental rebuilds cheap
(Chapter 3).

## 2.5 Layering rules

The codebase enforces a strict dependency order. A lower layer must never
include a higher one.

```
support      (arenas, diagnostics, string interner, file system, threads)
   ▲
syntax       (source manager, lexer, AST, parser)
   ▲
semantics    (name resolution, type system, ownership/borrow check)
   ▲
ir           (MIR construction, SSA, the optimizer)
   ▲
codegen      (LLVM lowering, target machine)
   ▲
driver       (mlangc: argument parsing, pipeline orchestration)
```

`tools/` and the language server sit beside the driver and depend on the layers
they need (`mfmt` on `syntax`, `mls` on `semantics`). The runtime is a separate
build with no dependency on the compiler.

## 2.6 Target and host separation

The compiler is a cross-compiler from day one. The *host* (the machine running
`mlangc`) and the *target* (the machine running the produced binary) are
distinct everywhere: type sizes, alignment, endianness, and calling conventions
come from a `TargetInfo` object, never from the host's `sizeof`. Supported
targets are listed in [Chapter 11](11-llvm-codegen.md) and
[Chapter 29](29-abi.md).
