# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project scaffolding: monorepo layout for compiler, runtime, stdlib,
  and developer tools.
- Complete language design document (`docs/design/`) spanning 39 chapters.
- Formal EBNF grammar (`docs/grammar/mlang.ebnf`).
- `mlangc` compiler frontend: source manager, diagnostics engine, lexer,
  AST hierarchy, and recursive-descent parser (C++23).
- Backend scaffolding: semantic analyzer, type checker, SSA-based IR, and an
  LLVM code-generation layer guarded behind the `MLANG_ENABLE_LLVM` option.
- `mlang-runtime` core: ownership + reference-counting allocator design and
  scheduler skeleton.
- Standard library modules authored in MLang (`stdlib/std`).
- Developer tools: `mfmt` (formatter), `mls` (language server), `mbuild`
  (build tool) scaffolding.
- VS Code extension *MLang Language Support*.
- Continuous integration workflows for Linux, macOS, and Windows.

[Unreleased]: https://github.com/mlang-org/mlang/commits/main
