# 30. FFI (Foreign Function Interface)

MLang interoperates with the existing native world through an explicit FFI. The
guiding rule: **interop is possible, visible, and unsafe by marker**, so the safe
core of the language is never silently compromised by a foreign call.

## 30.1 Calling C

Declaring and calling a C function:

```mlang
@extern("c")
unsafe fn c_strlen(s: *const Byte): USize       // from libc

@extern("c", link: "z")
unsafe fn compress(dst: *Byte, dstLen: *USize, src: *const Byte, srcLen: USize): Int

unsafe fn lengthOf(s: CString): USize {
    return c_strlen(s.ptr)
}
```

- `@extern("c")` gives the function the C ABI and an unmangled symbol
  (Chapter 29).
- `link:` records the native library to link; `mbuild` passes it to the linker.
- The declaration is `unsafe`: calling foreign code can violate MLang's
  guarantees (it can dangle, race, or corrupt memory), so the call site must be in
  an `unsafe` block, which makes every FFI crossing greppable.

## 30.2 The type bridge

A small set of FFI-compatible types map directly to C:

| MLang | C |
|-------|---|
| `i8..i64`, `u8..u64` | `int8_t..int64_t`, `uint8_t..uint64_t` |
| `f32`, `f64` | `float`, `double` |
| `bool` | `bool` |
| `*T`, `*const T` | `T*`, `const T*` |
| `@layout(c) struct` | a C `struct` with the same fields |
| `CString` | `const char*` (NUL-terminated) |
| function pointer type | C function pointer |

Non-FFI types (`String`, `List<T>`, `Rc<T>`, enums with payloads) do **not** cross
the boundary directly; you marshal them explicitly (e.g. `String.toCString()`),
which keeps the representation gap honest rather than hidden.

## 30.3 Exposing MLang to C (and thus to other languages)

```mlang
@export("c")
fn mlang_add(a: Int, b: Int): Int {
    return a + b
}
```

`@export("c")` emits an unmangled, C-callable symbol with the C ABI. A shared
library of such exports (Chapter 29) is consumable from C, C++, Python (ctypes),
Go (cgo), Rust (bindgen), and anything that speaks the C ABI. Ownership and
lifetime at the boundary are by documented contract, since C has no ownership
type system.

## 30.4 C++ interop

C++ has no stable ABI, so MLang does **not** bind C++ directly. The supported path
is the standard one: expose the C++ you need behind an `extern "C"` shim, then bind
that shim from MLang. A future `mango` helper (`mango bindgen`) will generate
MLang declarations from C headers to remove the boilerplate; a curated C++ subset
binding is a research item (Chapter 33), not a promise.

## 30.5 Callbacks and memory across the boundary

- **Callbacks**: a non-capturing MLang function can be passed as a C function
  pointer directly; a capturing closure is passed as a `{ fn, env }` pair with a
  C trampoline, and the environment's lifetime is the caller's responsibility
  (documented, `unsafe`).
- **Memory ownership**: who frees what is explicit. Memory allocated by C is freed
  by C (`unsafe`), memory allocated by MLang is freed by MLang; the boundary never
  guesses. Helpers (`withCString`, `Buffer.borrow`) scope foreign memory to a
  block so it is released deterministically.

## 30.6 Why FFI is `unsafe` and explicit

Every memory-safety guarantee in Chapter 13 rests on the compiler seeing all the
code. A foreign call is a hole in that knowledge. Making FFI `unsafe` and visible
means: the safe subset's guarantees still hold for all non-`unsafe` code, security
auditors can enumerate every trust boundary with one search, and the cost (manual
lifetime reasoning) is paid only exactly where you reach outside the language.
