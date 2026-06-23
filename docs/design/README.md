# MLang Design Document

This is the canonical design and specification reference for the MLang
programming language and its toolchain. It is organized as a book of 39
chapters. Each chapter is self-contained but cross-references the others.

The intent of this document is not marketing: every non-trivial decision is
accompanied by its technical rationale and, where relevant, the alternatives
that were considered and rejected.

## Table of contents

| #  | Chapter | Topic |
|----|---------|-------|
| 1  | [Language Philosophy](01-philosophy.md) | Goals, non-goals, guiding principles. |
| 2  | [Architecture](02-architecture.md) | High-level system architecture. |
| 3  | [Compiler Design](03-compiler-design.md) | Pipeline, threading, incrementality, caching. |
| 4  | [Lexer](04-lexer.md) | Tokenization, Unicode, error recovery. |
| 5  | [Parser](05-parser.md) | Grammar strategy, Pratt expressions, recovery. |
| 6  | [AST Structure](06-ast.md) | Node hierarchy, arenas, spans. |
| 7  | [Type System](07-type-system.md) | Nullability, generics, inference, variance. |
| 8  | [Semantic Analysis](08-semantic-analysis.md) | Name resolution, ownership, borrow checking. |
| 9  | [Intermediate Representation](09-ir.md) | MLIR/SSA design. |
| 10 | [Optimization Passes](10-optimization.md) | The pass pipeline and each transform. |
| 11 | [LLVM Code Generation](11-llvm-codegen.md) | Lowering MIR to LLVM IR. |
| 12 | [Runtime Architecture](12-runtime.md) | The mlang-runtime layout. |
| 13 | [Memory Management](13-memory-management.md) | Ownership + ARC hybrid. |
| 14 | [Why Not a Tracing GC](14-why-no-gc.md) | The rationale, with measurements. |
| 15 | [Concurrency Model](15-concurrency.md) | Structured concurrency, channels, memory model. |
| 16 | [Coroutine System](16-coroutines.md) | State machines, the scheduler. |
| 17 | [Exception System](17-exceptions.md) | Errors, panics, zero-cost unwinding. |
| 18 | [Generics](18-generics.md) | Monomorphization, constraints, variance. |
| 19 | [Reflection](19-reflection.md) | Metadata, runtime type info. |
| 20 | [Standard Library](20-stdlib.md) | Module map and design principles. |
| 21 | [Package Manager (mango)](21-mango.md) | Manifests, resolution, registry. |
| 22 | [Build Tool (mbuild)](22-mbuild.md) | Targets, the build graph, caching. |
| 23 | [Formatter (mfmt)](23-mfmt.md) | The canonical style algorithm. |
| 24 | [Language Server (mls)](24-mls.md) | LSP architecture. |
| 25 | [Debugger Architecture](25-debugger.md) | DWARF, the DAP adapter. |
| 26 | [Test Framework](26-test-framework.md) | Discovery, assertions, runner. |
| 27 | [File Organization](27-file-organization.md) | Source layout, modules, visibility. |
| 28 | [Project Structure](28-project-structure.md) | The shape of an MLang project. |
| 29 | [ABI & Binary Format](29-abi.md) | Calling convention, name mangling, metadata. |
| 30 | [FFI](30-ffi.md) | Interop with C and C++. |
| 31 | [Security Model](31-security.md) | Safety guarantees and the threat model. |
| 32 | [Performance Strategies](32-performance.md) | How MLang gets fast. |
| 33 | [Roadmap](33-roadmap.md) | Long-term direction. |
| 34 | [Grammar](34-grammar.md) | Notes on the formal EBNF. |
| 35 | [Example Programs](35-examples.md) | A guided tour of real MLang code. |
| 36 | [Compiler Class Diagram](36-compiler-class-diagram.md) | The C++23 module structure. |
| 37 | [Development Phases](37-development-phases.md) | MVP → Alpha → Beta → Stable. |
| 38 | [Benchmark Targets](38-benchmarks.md) | The performance bar, quantified. |
| 39 | [Repository Structure](39-repository-structure.md) | The professional repo layout. |

## Conventions used in this document

- **MUST / SHOULD / MAY** are used in the [RFC 2119](https://www.rfc-editor.org/rfc/rfc2119)
  sense when describing normative behavior.
- Code samples in `mlang` fences are MLang; `cpp` fences are compiler internals.
- "the compiler" means `mlangc` unless otherwise noted.
- Grammar fragments use the EBNF dialect defined in [Chapter 34](34-grammar.md).
