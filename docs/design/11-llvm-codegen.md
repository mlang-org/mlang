# 11. LLVM Code Generation

The backend lowers optimized MIR to LLVM IR, then asks LLVM to produce native
objects. LLVM was chosen for its mature optimizer, broad target coverage, and
excellent debug-info and exception-handling support. The backend is isolated
behind an interface so a future self-hosted native backend can be slotted in
(Chapter 33).

## 11.1 Why LLVM (and the cost of the choice)

- **Pros**: world-class instruction selection and register allocation; x86-64,
  AArch64, ARM, RISC-V, WebAssembly out of the box; battle-tested DWARF and
  unwind tables; `lld` for fast linking; sanitizers for free during development.
- **Cons**: a large build/runtime dependency, slow `-O0` link if misused, and an
  IR that does not understand MLang. We accept these by (a) gating LLVM behind a
  build option so the frontend and tools do not require it, (b) doing the
  language-aware optimization on MIR first, and (c) keeping the lowering thin.

## 11.2 The lowering interface

```cpp
class CodeGenBackend {
public:
    virtual ~CodeGenBackend() = default;
    virtual void lowerModule(const mir::Module&, const TargetInfo&) = 0;
    virtual void emitObject(std::string_view path) = 0;
    virtual void emitAssembly(std::string_view path) = 0;
};
```

`LlvmBackend` implements this. Because the rest of the compiler depends only on
`CodeGenBackend`, building without LLVM simply omits `LlvmBackend` and the driver
reports "native codegen unavailable" while everything up to `--emit=ir` still
works. This is what lets the project build and the tests run on a stock C++23
toolchain.

## 11.3 Mapping MIR to LLVM IR

| MIR | LLVM IR |
|-----|---------|
| MIR `Function` | `llvm::Function` with the MLang calling convention (Chapter 29) |
| MIR `Block` | `llvm::BasicBlock` |
| SSA value | LLVM SSA value (one-to-one; both are SSA) |
| `phi` | `llvm::PHINode` |
| value type / struct | `llvm::StructType` (laid out per `TargetInfo`) |
| reference type | pointer in the appropriate address space |
| `retain`/`release` | calls to runtime intrinsics, or inlined atomic ops |
| `call_virtual` | vtable load + indirect call |
| `unwind` edge | `invoke` + landing pad (Chapter 17) |
| `switch` over enum tag | `llvm::SwitchInst`, often a jump table |
| MIR `nonnull`/`noalias`/`readonly` | LLVM parameter/return attributes |

Because MIR is already SSA and already optimized, this mapping is close to
one-to-one and carries the MIR analysis results across as LLVM attributes, so
LLVM does not have to re-derive (for example) that a pointer is non-null.

## 11.4 The LLVM pass pipeline

After lowering, the backend runs LLVM's `default<O2>` (or `O3`) pipeline via the
new pass manager. We deliberately do *not* duplicate MIR passes here; LLVM's job
is target-specific: instruction selection, register allocation, scheduling,
peephole, and target tuning. For `-flto`, ThinLTO is used (scalable, parallel,
cache-friendly) over full LTO.

## 11.5 Targets

| Triple | Status |
|--------|--------|
| `x86_64-unknown-linux-gnu` | tier 1 |
| `aarch64-unknown-linux-gnu` | tier 1 |
| `x86_64-pc-windows-msvc` | tier 1 |
| `aarch64-apple-darwin` | tier 1 |
| `x86_64-apple-darwin` | tier 2 |
| `armv7-unknown-linux-gnueabihf` | tier 2 |
| `riscv64-unknown-linux-gnu` | tier 3 |
| `wasm32-unknown-unknown` | tier 3 (planned, Chapter 33) |

Tier 1 means the full test suite runs on it in CI and releases are gated on it.
Target details (sizes, alignment, calling convention) come from `TargetInfo`, not
the host (Chapter 2).

## 11.6 Debug info

The backend emits DWARF (ELF/Mach-O) or CodeView (PE) so that native debuggers
and the MLang DAP adapter (Chapter 25) can map machine state back to MLang source:
line tables, lexical scopes, variable locations, and types (including generics
rendered with their instantiated arguments). Debug info is emitted at every
optimization level; at `-O2`+ it is best-effort where optimization has dissolved
a variable, with `is_stmt` markers tuned for a good stepping experience.

## 11.7 Linking and startup

The backend produces objects; the driver invokes the system linker (`lld`
preferred) to produce the final image, linking against `mlang-runtime`
(Chapter 12). The runtime provides `_mlang_start`, which sets up the scheduler and
the main coroutine and then calls the user `main`. Static linking is the default
for self-contained binaries; dynamic linking against a shared runtime is an
option for size-sensitive deployments.
