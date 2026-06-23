# 39. Repository Structure

This chapter documents the layout of the MLang repositories themselves: what lives
where and why. It is the map for a new contributor (Chapter 28 covers a *user's*
project; this covers *our* project).

## 39.1 The two repositories

MLang is developed across two repositories under the `mlang-org` organization:

- **`mlang-org/mlang`** - the monorepo for the compiler, runtime, standard
  library, developer tools, docs, examples, and tests.
- **`mlang-org/mango`** - the package manager, developed independently and linked
  into the monorepo as a git submodule at `packages/mango`.

`mango` is separate because it has a different release cadence and audience (it is
also used standalone) and a smaller, self-contained codebase. The submodule keeps
the monorepo able to build the whole toolchain while letting `mango` version
itself.

## 39.2 The `mlang` monorepo

```
mlang/
├── README.md  LICENSE  CONTRIBUTING.md  CODE_OF_CONDUCT.md  CHANGELOG.md
├── CMakeLists.txt  CMakePresets.json     # top-level build (Chapter 22 is mbuild; this is the compiler's own build)
├── .clang-format  .editorconfig  .gitignore  .gitattributes
├── .github/workflows/                    # CI: build, test, format-check, sanitizers
├── cmake/                                # shared CMake modules (warnings, LLVM find)
├── docs/
│   ├── design/                           # this 39-chapter design document
│   ├── grammar/mlang.ebnf                # the formal grammar (Chapter 34)
│   ├── spec/                             # the normative spec (grows from design, Ch.33)
│   └── adr/                              # architdecture decision records
├── compiler/                            # mlangc - the C++23 compiler
│   ├── include/mlang/{support,lexer,ast,parser,sema,types,ir,codegen,driver,diagnostics}/
│   ├── src/{...same...}/
│   └── tests/                            # compiler unit tests
├── runtime/                             # mlang-runtime (Chapter 12)
│   ├── include/mlang_rt/
│   └── src/
├── stdlib/                              # the standard library, in MLang (Chapter 20)
│   └── std/{core,io,collections,string,math,time,concurrent,json,http,fs,reflect,crypto,regex,log,test}/
├── tools/
│   ├── mfmt/                             # formatter (Chapter 23)
│   ├── mls/                              # language server (Chapter 24)
│   └── mbuild/                           # build tool (Chapter 22)
├── editors/vscode/                      # "MLang Language Support" extension (Chapter 24)
├── examples/                            # the tour programs (Chapter 35)
├── tests/                               # cross-cutting + end-to-end tests
│   └── {lexer,parser,sema,codegen,e2e}/
├── scripts/                             # dev/release/CI helper scripts
└── packages/mango/                      # git submodule -> mlang-org/mango
```

## 39.3 The `mango` repository

```
mango/
├── README.md  LICENSE  CHANGELOG.md
├── CMakeLists.txt
├── include/mango/                       # manifest, lockfile, resolver, registry client
├── src/
├── docs/                                # manifest format, resolution algorithm, registry API
└── tests/
```

## 39.4 Why a monorepo for the toolchain

The compiler, runtime, stdlib, and tools change together (a new language feature
touches the parser, the type checker, the IR, the backend, the stdlib, and `mls`).
A monorepo makes such a change one atomic, reviewable, CI-gated commit, and lets
the tools share `libmlang_frontend` (Chapter 2) without cross-repo version
juggling. The package manager is the one component with an independent life, so it
is the one component split out.

## 39.5 Branching, CI, and releases

- **Branching**: trunk-based on `main`; short-lived feature branches; PRs require
  a green CI and review (Chapter, CONTRIBUTING).
- **CI** (`.github/workflows/`): configure + build + test on the tier-1 matrix,
  `mfmt --check`, clang-format check, sanitizer and fuzz jobs, and (nightly) the
  benchmark suite (Chapter 38).
- **Releases**: semantic-versioned, tagged, with prebuilt toolchains per platform;
  the submodule pin in `mlang` records the exact `mango` version in a release.
- **Commits**: Conventional Commits (CONTRIBUTING.md), one logical change each,
  with the changelog updated for notable entries.

## 39.6 Reading order for a new contributor

1. This chapter (the map) and Chapter 2 (the architecture).
2. Chapters 4-6 to understand the frontend, then `compiler/src/{lexer,parser}`.
3. The example programs (Chapter 35) to learn the language.
4. CONTRIBUTING.md for the build and the workflow.
5. The chapter for whatever subsystem you are touching, then its `src/` directory.
