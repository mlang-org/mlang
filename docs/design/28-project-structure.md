# 28. Project Structure

This chapter describes the shape of a *user's* MLang project (what `mbuild new`
scaffolds), as distinct from the structure of the MLang repository itself
(Chapter 39).

## 28.1 A new application

```
myapp/
├── mango.json          # manifest: name, version, deps, targets (Chapter 21)
├── mango.lock          # pinned dependency versions (committed)
├── mfmt.json           # optional formatter knobs (Chapter 23)
├── README.md
├── .gitignore          # ignores build/, packages/
├── src/
│   └── main.m          # entry point: public fn main()
├── tests/
│   └── main_test.m
└── (generated)
    ├── build/          # mbuild output (git-ignored)
    └── packages/       # fetched deps, reproducible from the lock (git-ignored)
```

## 28.2 A library

```
mylib/
├── mango.json          # target kind: library
├── mango.lock
├── src/
│   └── lib.m           # the public surface (re-exports)
├── tests/
└── examples/           # runnable usage examples, also compiled in CI
```

A library declares a `library` target whose `entry` is its root module; the
`public` symbols reachable from that root form the published API (the `.mmi`,
Chapter 2). `examples/` programs are compiled by CI so documentation examples
cannot rot.

## 28.3 The entry point

```mlang
namespace com.example.myapp

import std.io.Console

public fn main() {
    Console.println("Hello, MLang")
}
```

`main` may be:

- `fn main()` - returns nothing, exit code 0 on normal return.
- `fn main(): Int` - the return value is the process exit code.
- `fn main(args: [String]): Int` - receives command-line arguments.
- `async fn main()` - runs on the scheduler; the runtime drives it to completion
  and joins outstanding structured-concurrency children before exit (Chapter 12).

## 28.4 Multi-target and workspace projects

A single `mango.json` can declare several targets (an executable plus a library
plus benchmarks). For larger systems, a **workspace** groups multiple packages
under one root with a shared lockfile:

```
workspace/
├── mango.json          # { "workspace": ["packages/*"] }
├── mango.lock          # one lock for all members
└── packages/
    ├── core/   (mango.json)
    ├── server/ (mango.json, depends on core)
    └── cli/    (mango.json, depends on core)
```

Workspace members share dependency resolution (one consistent version set) and
build cache, and can depend on each other by path. This is how a monorepo of
related MLang packages is organized.

## 28.5 Conventions the tooling assumes

- Source under `src/`, tests under `tests/`, examples under `examples/`.
- `main.m` (app) or `lib.m` (library) as the conventional entry module.
- `mango.lock` is committed; `build/` and `packages/` are not.
- One package per `mango.json`; one logical product per package.

These conventions let `mbuild` and `mls` work with zero configuration for the
common case while remaining overridable in `mango.json` for the unusual one.
