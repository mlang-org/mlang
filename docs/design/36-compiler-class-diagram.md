# 36. Compiler Class Diagram and Module Structure

This chapter describes the C++23 structure of `mlangc`: the modules, the key
classes in each, and how they depend on one another. It is the bridge between the
conceptual pipeline (Chapter 3) and the source tree (`compiler/`).

## 36.1 Module dependency graph

The layers from Chapter 2, with their C++ namespaces and directories:

```
mlang::support   (compiler/src/support)      ── arenas, diagnostics, strings, threads
        ▲
mlang::syntax    (lexer, ast, parser)        ── SourceManager, Lexer, Parser, AST
        ▲
mlang::sema      (sema, types)               ── Resolver, TypeChecker, OwnershipChecker
        ▲
mlang::ir        (ir)                         ── MirBuilder, MirModule, PassManager
        ▲
mlang::codegen   (codegen)                    ── CodeGenBackend, LlvmBackend
        ▲
mlang::driver    (driver, main.cpp)          ── CompilerInvocation, Compiler, mlangc
```

A class in a layer may use its own and lower layers only. This is enforced by
include-path discipline and checked in CI.

## 36.2 `mlang::support`

| Class | Responsibility |
|-------|----------------|
| `Arena` | bump allocator for AST/MIR nodes (Chapter 6) |
| `StringInterner` | concurrent symbol table; `Symbol` is a 32-bit id (Chapter 4) |
| `SourceManager` | owns files, maps `SourceLoc` to file/line/col (Chapter 4) |
| `DiagnosticEngine` | builds, dedups, sorts, and renders diagnostics (Chapter 8) |
| `Diagnostic`, `DiagnosticBuilder` | a single message with spans, notes, fixes |
| `ThreadPool` | work-stealing pool for parallel compilation (Chapter 3) |
| `TargetInfo` | sizes/alignment/convention for the target (Chapter 2) |

## 36.3 `mlang::syntax`

| Class | Responsibility |
|-------|----------------|
| `Token`, `TokenKind` | the 16-byte token (Chapter 4) |
| `Lexer` | byte stream to token stream, error-recovering (Chapter 4) |
| `Node`, `Decl`, `Stmt`, `Expr`, `TypeRef` | the AST hierarchy (Chapter 6) |
| `AstModule` | a parsed file's top-level declarations |
| `Parser` | recursive descent + Pratt expressions (Chapter 5) |
| `AstVisitor<R>`, `AstWalker` | generated traversal (Chapter 6) |
| `AstPrinter` | dumps AST for `--emit=ast`; full-fidelity printer for `mfmt` |

The node list `ast/Nodes.def` is the single source of truth from which visitors,
the walker, and the serializer are generated.

## 36.4 `mlang::sema`

| Class | Responsibility |
|-------|----------------|
| `ScopeTree`, `Scope` | hierarchical name scopes (Chapter 8) |
| `Resolver` | two-pass name resolution (Chapter 8) |
| `Type`, `TypeInterner` | interned semantic types (Chapter 7) |
| `TypeChecker` | bidirectional inference, overloads, conformance (Chapter 7) |
| `FlowAnalysis` | smart-casts, definite assignment (Chapter 8) |
| `OwnershipChecker` | move/borrow analysis, ARC/drop insertion (Chapter 13) |
| `ConstEvaluator` | compile-time evaluation (Chapter 8) |
| `ModuleInterface` | reads/writes `.mmi` (Chapter 2) |

## 36.5 `mlang::ir`

| Class | Responsibility |
|-------|----------------|
| `mir::Module`, `mir::Function`, `mir::Block`, `mir::Inst` | SSA IR (Chapter 9) |
| `MirBuilder` | lowers typed AST to SSA MIR (Chapter 9) |
| `MirVerifier` | SSA/type/ownership well-formedness (Chapter 9) |
| `MirPass`, `PassManager` | the optimization pipeline (Chapter 10) |
| `AnalysisManager` | cached dominators/alias/loop/liveness (Chapter 10) |
| concrete passes | `Inliner`, `EscapeAnalysis`, `ArcOpt`, `Devirt`, `Bce`, `LoopOpt`, ... |

## 36.6 `mlang::codegen`

| Class | Responsibility |
|-------|----------------|
| `CodeGenBackend` | the abstract lowering/emit interface (Chapter 11) |
| `LlvmBackend` | MIR to LLVM IR, runs LLVM passes, emits objects (Chapter 11) |
| `DebugInfoBuilder` | DWARF/CodeView emission (Chapter 25) |
| `Mangler` / `Demangler` | symbol mangling (Chapter 29) |

`LlvmBackend` is compiled only when `MLANG_ENABLE_LLVM=ON`; otherwise the driver
runs up to `--emit=ir`.

## 36.7 `mlang::driver`

| Class | Responsibility |
|-------|----------------|
| `CompilerInvocation` | a fully-resolved, serializable job description (Chapter 3) |
| `Compiler` | orchestrates the pipeline for one invocation |
| `ArgParser` | command-line parsing |
| `main` | the `mlangc` entry point |

## 36.8 Design conventions in the C++

- **C++23**, with modules where the toolchain supports them, otherwise header/TU.
- **No exceptions for control flow** inside the compiler; fallible operations
  return `Expected<T>`/`std::expected`, mirroring the language's own philosophy.
- **Value semantics and arenas** over shared ownership; few `shared_ptr`s.
- **`std::span`/`std::string_view`** at boundaries to avoid copies.
- **Generated code** (visitors, serializers) from `.def` files so adding a node or
  diagnostic updates every switch and fails to compile until exhaustively handled.
- **Determinism** is a tested invariant (Chapter 3): no iteration over unordered
  containers in output-affecting positions without a sort.
