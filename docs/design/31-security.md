# 31. Security Model

Security in MLang is layered: **language-level memory and type safety**,
**runtime hardening**, and **supply-chain integrity** in the toolchain. This
chapter states the guarantees, the threat model, and the boundaries.

## 31.1 Language-level guarantees (safe subset)

In code that contains no `unsafe`, the compiler guarantees:

- **Memory safety**: no use-after-free, double-free, dangling pointer, or buffer
  overflow (Chapter 13). Array/slice access is bounds-checked; out-of-bounds is a
  panic, not a corruption (Chapter 17).
- **Type safety**: no type confusion; every cast is checked or statically proven.
- **Null safety**: no null-dereference; absence is a typed `T?` that must be
  handled (Chapter 7).
- **Data-race freedom**: no data races (Chapter 15).
- **Initialization safety**: no reads of uninitialized memory (definite-assignment
  analysis, Chapter 8).
- **Integer-overflow defined behavior**: checked (panic) by default in debug;
  explicit wrapping operators otherwise. There is no undefined behavior on
  overflow.

These eliminate, by construction, the majority of the historical CVE categories
(memory corruption, type confusion, races).

## 31.2 The `unsafe` boundary

`unsafe` is the single, auditable escape hatch. It enables raw pointer
dereference, calling `unsafe`/FFI functions (Chapter 30), unchecked casts, and
manual lifetime control. Properties:

- Every relaxation of the rules requires the `unsafe` keyword, so the entire trust
  boundary of a codebase is `grep -r unsafe`.
- `unsafe` does not turn off the *rest* of the checks; it permits a specific set
  of operations, and the surrounding safe code still gets full guarantees.
- The standard library encapsulates its `unsafe` inside safe APIs, with each
  `unsafe` block documented with the invariant it upholds. Auditing the library's
  safety reduces to auditing those blocks.

## 31.3 Runtime hardening

Independent of language safety, the runtime and codegen enable standard
mitigations:

- Stack canaries, ASLR/PIE, and non-executable stack/heap (`W^X`) by default on
  supported targets.
- Control-flow integrity (CFI) and shadow stacks where the target supports them.
- Allocator hardening: size-class isolation, guard pages for large allocations,
  and zero-on-free for sensitive types (`@zeroize`, used by `crypto`).
- Sanitizer builds (ASan/UBSan/TSan) available for `unsafe`/FFI-heavy code during
  development.

## 31.4 Supply-chain security

The toolchain treats dependencies as untrusted by default (Chapter 21):

- **Locked + checksummed** dependencies; a hash mismatch is a hard failure.
- **No install/build scripts execute by default** - the most common package-
  manager compromise vector is closed; build steps are declarative.
- **Signed packages** and a verifiable registry; `mango audit` surfaces known
  advisories.
- **Reproducible builds** (Chapter 22) let a third party verify that a published
  binary matches its source.
- **Capability-scoped builds** (roadmap, Chapter 33): declaring which
  capabilities (filesystem, network) a dependency may use at build time, to
  contain a malicious transitive dependency.

## 31.5 Threat model

MLang's safety guarantees defend against: memory-corruption exploitation, type
confusion, data races, and a class of logic bugs (unhandled null, non-exhaustive
match, ignored errors) that the type system turns into compile errors. They do
**not** by themselves defend against: logic errors within safe code, side
channels (timing/cache - `crypto` provides constant-time primitives for those),
misuse of `unsafe`/FFI, or a compromised toolchain/host. Those are addressed by
hardening, the constant-time library, the `unsafe` audit surface, and reproducible
builds respectively, and are stated honestly rather than over-claimed.

## 31.6 Security process

The project commits to a documented vulnerability-disclosure process, a security
advisory channel, prompt patching of the compiler/runtime/stdlib, and CI that
runs the sanitizers and the fuzzers (Chapter 10) on every change, so that the
compiler that promises safety is itself held to that standard.
