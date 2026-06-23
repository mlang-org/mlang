# 29. ABI and Binary Format

This chapter defines how MLang code is laid out and called at the machine level:
data layout, the calling convention, name mangling, the metadata sections, and
the stability guarantees around them.

## 29.1 ABI stability stance

MLang commits to a **stable C ABI at the FFI boundary** (Chapter 30) and an
**unstable internal ABI** between MLang modules within a compiler version. That
is: two MLang modules must be built by the same compiler version to link
(enforced by a version stamp in the `.mmi` and object), but anything exposed
`extern "C"` follows the platform C ABI forever. This is the Rust stance, chosen
because a frozen internal ABI would prevent layout optimization; the `.mmi`
version check turns an ABI mismatch into a clean error instead of a crash.

## 29.2 Data layout

- **Primitives** use the target's natural size and alignment from `TargetInfo`
  (Chapter 2): `i32` is 4 bytes, `f64` is 8, pointers are the target word.
- **Structs** are laid out with fields reordered by default to minimize padding
  (smallest alignment last), unless declared `@layout(c)` for FFI, which forces
  C field order. Size and alignment are the max-of-fields rules.
- **Enums** with payloads are tagged unions; the tag is the smallest integer that
  fits the case count, and **niche optimization** packs the tag into a payload's
  unused bit patterns where possible (so `T?` for a reference type is just a
  possibly-null pointer, no extra word).
- **References** are pointers; `Rc<T>` is a pointer to a header (`{ strong,
  weak, type-info }`) followed by the payload; `Weak<T>` points at the same
  header without owning the payload (Chapter 13).
- **Closures** are a pair `{ fn-pointer, environment-pointer }`; non-capturing
  closures degrade to a bare function pointer.

## 29.3 Calling convention

The default MLang calling convention is built on the platform's C convention
(System V AMD64, AArch64 AAPCS, Windows x64) with MLang-specific rules:

- Small value types are passed in registers (scalarized); large ones by hidden
  pointer.
- The error/`Result` return uses a two-register `{ value, tag }` return where it
  fits, avoiding a heap box for the common small-error case.
- `async` functions take a hidden coroutine-frame pointer and follow the
  poll/resume signature (Chapter 16).
- `self`/`this` is the first argument for methods.
- Ownership transfer is encoded in the convention: an owned argument is the
  callee's responsibility to drop; a borrowed argument is not. This is invisible
  at the C boundary (where everything is `extern "C"` and ownership is by
  convention/documentation).

## 29.4 Name mangling

MLang symbols are mangled to encode namespace, name, generic arguments, and
signature, so overloads and instantiations get distinct linker symbols and a
demangler can reconstruct the source name for backtraces and debuggers:

```
_M  <namespace>  <name>  <generic-args>  <param-types>  E
```

The scheme is documented and stable per compiler version; `mlangc --demangle`
and the debugger/`mls` reverse it for human-readable stacks. `extern "C"` symbols
are *not* mangled (they keep their declared name) so C can call them.

## 29.5 Binary metadata sections

A compiled MLang object/image carries dedicated sections:

| Section | Contents | Consumer |
|---------|----------|----------|
| `.mlang.types` | reflection metadata (Chapter 19) for `@reflect` types | runtime/reflection |
| `.mlang.modinfo` | module name, compiler version, feature flags | linker check, tooling |
| `.eh_frame`/SEH | unwind tables (Chapter 17) | runtime unwinder, debuggers |
| `.debug_*` | DWARF/CodeView debug info (Chapter 25) | debuggers |

The version stamp in `.mlang.modinfo` is what produces a clean "built with a
different compiler version" error rather than an undefined-behavior link.

## 29.6 Executable and library formats

MLang targets the platform-native formats: ELF (Linux), Mach-O (macOS), PE/COFF
(Windows). It produces executables, static libraries (`.a`/`.lib`), and shared
libraries (`.so`/`.dylib`/`.dll`). A shared library can export a C ABI surface
(Chapter 30) so MLang code is consumable from other languages.

## 29.7 Versioning of the formats

The internal `.mmi` format and mangling carry a format version; the compiler
refuses to read a `.mmi` from an incompatible version with a clear message. The
public C ABI surface is governed by semantic versioning of the *library*, not the
compiler, so a library can keep a stable C API across compiler upgrades.
