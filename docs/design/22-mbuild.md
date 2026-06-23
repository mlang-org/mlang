# 22. Build Tool (`mbuild`)

`mbuild` orchestrates compilation. It turns "a project + its locked dependencies"
into "artifacts", incrementally and in parallel, by driving `mango` (for sources)
and `mlangc` (for compilation) over a build graph.

## 22.1 Responsibilities

- Discover targets from `mango.json` (executables, libraries, tests, benches).
- Construct the **build DAG**: modules within the project plus all dependency
  packages, ordered by their `import` edges and `.mmi` dependencies (Chapter 2).
- Schedule compilation across a thread pool, respecting the DAG.
- Cache results (content-addressed, Chapter 3) and rebuild only what changed.
- Invoke the linker to produce the final image, linking `mlang-runtime`.
- Run tests, benchmarks, and post-build steps.

## 22.2 The commands

| Command | Effect |
|---------|--------|
| `mbuild build [--release]` | Build all targets (or a named one). |
| `mbuild run [target] -- args` | Build then run an executable target. |
| `mbuild test [filter]` | Build and run the test target(s). |
| `mbuild bench [filter]` | Build and run benchmarks. |
| `mbuild check` | Type-check only (no codegen); fast feedback. |
| `mbuild clean` | Remove build artifacts. |
| `mbuild package` | Produce a distributable artifact. |
| `mbuild publish` | Hand off to `mango publish`. |
| `mbuild new <name>` / `init` | Scaffold a new project. |

`mbuild check` is deliberately separate and fast: it runs the frontend only and
skips the backend, which is the inner loop for editing (and what `mls` uses).

## 22.3 The build graph and incrementality

The unit of compilation is the module; the unit of *caching* is the function
(Chapter 3). `mbuild` computes a fingerprint for each module from its sources, its
dependencies' `.mmi` fingerprints, the compiler version, the target, and the
flags. A module whose fingerprint is unchanged is served from cache. Because a
body-only edit does not change a module's `.mmi` (Chapter 2), it does not
invalidate downstream modules - only the edited module recompiles. This is what
keeps large-project rebuilds proportional to the change, not the project.

## 22.4 Profiles

```json
"profiles": {
  "dev":     { "opt": 0, "debug": true,  "checks": "all" },
  "release": { "opt": 2, "debug": true,  "checks": "overflow-off", "lto": "thin" },
  "dist":    { "opt": 3, "debug": false, "lto": "full", "strip": true }
}
```

A profile bundles optimization level, debug info, runtime checks, and LTO. `dev`
favors compile speed and full checks; `release`/`dist` favor runtime speed. The
profile is part of the cache key, so switching profiles does not corrupt the
cache.

## 22.5 Relationship to `mango` and `mlangc`

`mbuild` is the conductor; it does not resolve versions (that is `mango`) and does
not compile (that is `mlangc`). It calls `mango` to ensure `packages/` matches the
lock, then walks the DAG invoking `mlangc` per module with a precise
`CompilerInvocation`, then links. Keeping these three tools separate (resolve /
build / compile) means each is testable in isolation and replaceable; a CI system
can call `mlangc` directly if it wants to manage scheduling itself.

## 22.6 Reproducibility

Given the same lockfile, compiler version, and profile, `mbuild` produces
byte-identical artifacts (the compiler is deterministic, Chapter 3; paths are
remapped to be build-location-independent). Reproducible builds are a security
property (Chapter 31) and a caching enabler (a teammate's cache entry is valid for
you).
