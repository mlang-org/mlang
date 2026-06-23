# 33. Roadmap

This chapter describes the long-term direction of the language and toolchain. The
near-term, milestone-by-milestone delivery plan is in
[Chapter 37](37-development-phases.md); this is the multi-year arc.

## 33.1 Language evolution

- **Const generics** (Chapter 18): `struct Matrix<T, const R: Int, const C: Int>`,
  enabling dimension-checked numerics and fixed-capacity collections.
- **`comptime` metaprogramming** (Chapter 19): compile-time code execution and
  type introspection to derive serializers, ORMs, and DI wiring with zero runtime
  reflection cost.
- **Effect annotations**: a lightweight effect system to track `async`, `throws`,
  allocation, and capability effects in signatures, enabling stronger purity and
  capability guarantees.
- **Pattern matching extensions**: or-patterns, range patterns, guard refinement,
  and destructuring assignment.
- **Linear/affine resource types**: a `must-use, must-consume` annotation for
  protocol-state types (e.g. a `File` that must be closed), building on ownership.

## 33.2 Memory model evolution

- **Region/arena APIs** (Chapter 14): a third point on the memory curve for
  bulk-lifetime workloads (request-scoped arenas), complementing ownership+ARC.
- **Borrow-checker ergonomics**: non-lexical, more precise borrow regions to
  accept more correct programs without annotation.
- **Cycle-collector improvements**: making the opt-in collector (Chapter 13) fully
  concurrent and incremental for the workloads that need it.

## 33.3 Backend evolution

- **A native (non-LLVM) backend** behind the existing `CodeGenBackend` interface
  (Chapter 11), for fast `-O0` builds and to remove the LLVM build dependency for
  common targets.
- **WebAssembly** as a tier-1 target (Chapter 11), including the component model,
  to run MLang in the browser and in WASI environments.
- **GPU/accelerator targets** for data-parallel kernels (research).
- **Cranelift-style fast baseline** for the dev loop, with LLVM reserved for
  release.

## 33.4 Tooling evolution

- **Distributed/remote build cache** (Chapter 3): the content-addressed cache made
  shareable across a team and CI.
- **`mango` registry maturity**: mirroring, private registries, organization
  policies, and capability-scoped dependencies (Chapter 31).
- **First-class refactoring** in `mls` (Chapter 24): cross-module rename, extract,
  and codemods.
- **A package documentation site generator** from `///` doc comments.
- **A REPL / scripting front end** for exploration and teaching.

## 33.5 Ecosystem and platform

- Tier-1 support across Linux, macOS, Windows on x86-64 and AArch64, broadening to
  RISC-V and embedded profiles.
- A curated standard ecosystem (web frameworks, database drivers, serialization)
  built and versioned with the language.
- An interop story for the dominant ecosystems via the C ABI (Chapter 30) and
  generated bindings.

## 33.6 Governance and stability

- A public RFC process for language and library changes.
- A stability commitment: once 1.0 ships, source compatibility within a major
  version, with an **edition** mechanism (declared in `mango.json`) to introduce
  opt-in breaking changes without splitting the ecosystem.
- A specification track: this design document graduates into a versioned, normative
  language specification.

## 33.7 Guiding constraint

Every item above is gated on the invariant that the language stays **safe by
default, predictable in cost, and small in surface**. Features that would
compromise the memory-safety guarantees, the readable cost model, or the "one
obvious way" tooling are out of scope no matter how popular, because those three
properties are the reason MLang exists (Chapter 1).
