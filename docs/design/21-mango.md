# 21. Package Manager (`mango`)

`mango` manages dependencies: declaring them, resolving a consistent version set,
fetching them, and publishing your own. It is to MLang what Cargo is to Rust. It
lives in its own repository (`mlang-org/mango`) and is linked into the main repo
as a submodule (`packages/mango`).

## 21.1 The manifest: `mango.json`

A project (a "package") is any directory with a `mango.json`:

```json
{
  "name": "com.example.app",
  "version": "1.2.0",
  "edition": "2026",
  "description": "An example MLang application.",
  "license": "MIT",
  "authors": ["Jane Dev <jane@example.com>"],
  "entry": "src/main.m",
  "dependencies": {
    "std.http": "^1.4.0",
    "com.acme.json": ">=2.0.0 <3.0.0",
    "com.local.utils": { "path": "../utils" },
    "com.git.lib":     { "git": "https://github.com/x/lib.git", "tag": "v0.9.1" }
  },
  "devDependencies": {
    "com.test.bench": "^0.5.0"
  },
  "targets": {
    "app": { "kind": "executable", "entry": "src/main.m" },
    "lib": { "kind": "library",    "entry": "src/lib.m" }
  }
}
```

JSON is chosen for the manifest because it is universal, has first-class tooling,
and is unambiguous; a stricter superset (comments, trailing commas) is accepted on
read and normalized on write by `mango`.

## 21.2 Version resolution

`mango` uses **semantic versioning** with caret/range constraints and produces a
`mango.lock` pinning every transitive dependency to an exact version + content
hash. Resolution:

- Builds the dependency graph from all manifests.
- Selects versions by a deterministic algorithm (a SAT/PubGrub-style solver) that
  finds a set satisfying all constraints, preferring the newest compatible
  versions, and produces clear, actionable errors on conflict ("package A needs
  json ^1, package B needs json ^2").
- Writes the lockfile so builds are reproducible across machines and time.

Unlike `npm`'s nested duplication, `mango` resolves to a single version per
package per major version where possible, and allows multiple *majors* to coexist
(they are different packages to the type system, via the version in the module
path), which avoids both diamond conflicts and silent breakage.

## 21.3 The commands

| Command | Effect |
|---------|--------|
| `mango install` | Resolve, fetch, and lock dependencies into `packages/`. |
| `mango add <pkg>[@ver]` | Add a dependency and update the manifest + lock. |
| `mango remove <pkg>` | Remove a dependency. |
| `mango update [pkg]` | Re-resolve within constraints; bump the lock. |
| `mango search <query>` | Search the registry. |
| `mango publish` | Package and upload to the registry (after `mango login`). |
| `mango tree` | Print the resolved dependency graph. |
| `mango why <pkg>` | Explain why a package is in the graph. |

## 21.4 The registry

Packages are published to a registry (default `registry.mlang.org`, configurable
for private/self-hosted use). Design points:

- **Content-addressed and immutable.** A published `name@version` is permanent
  (yanking marks it unusable for new resolutions but never deletes it), so a build
  that worked yesterday works tomorrow.
- **Checksums verified on download** against the lockfile; a mismatch is a hard
  error (supply-chain integrity, Chapter 31).
- **Namespaced names** (`com.acme.json`) prevent typosquatting on short names and
  mirror the language's namespace system.
- **Offline mode** uses the local cache; CI can vendor `packages/` for fully
  hermetic builds.

## 21.5 The local layout

```
project/
├── mango.json
├── mango.lock
├── src/
└── packages/            # fetched dependencies (git-ignored, reproducible from lock)
    ├── std.http-1.4.2/
    └── com.acme.json-2.1.0/
```

`mbuild` (Chapter 22) reads the lock and the resolved `packages/` to construct the
build graph; `mango`'s job ends at "the right sources are on disk and pinned".

## 21.6 Security

`mango` verifies checksums, supports signed packages, runs no install-time
scripts by default (a major npm attack vector is thereby closed), and reports
known-vulnerability advisories via `mango audit`. The threat model is in
[Chapter 31](31-security.md).
