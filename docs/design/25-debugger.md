# 25. Debugger Architecture

MLang debugging is built on **standard native debug info** plus a thin
**Debug Adapter Protocol (DAP)** layer, so existing debuggers (LLDB, GDB) and
every DAP-capable editor work out of the box, with an MLang-aware presentation on
top.

## 25.1 Strategy: stand on standards

Rather than write a debugger from scratch, MLang emits the debug info that mature
debuggers already consume (DWARF on ELF/Mach-O, CodeView/PDB on Windows) and
provides a small adapter that speaks DAP. This gives breakpoints, stepping,
stack traces, and variable inspection on day one across Linux, macOS, and
Windows, and lets users keep their existing tools.

## 25.2 Debug info emission

The LLVM backend (Chapter 11) emits, at every optimization level:

- Line tables mapping machine addresses to `.m` source locations, with `is_stmt`
  markers tuned for natural stepping.
- Lexical scopes and variable locations (registers/stack slots/expressions), kept
  accurate where optimization allows and marked `<optimized out>` where it does
  not.
- Type descriptions, including generics rendered with their instantiated
  arguments (`List<Int>`, not `List<T>`), nullable types, enums with their cases,
  and closures with their captured environment.

At `-O0` debugging is faithful (every variable is live and inspectable); at
`-O2`+ it is best-effort, which is the standard, accepted trade.

## 25.3 The DAP adapter

A small adapter translates between DAP (what editors speak) and the underlying
debugger (LLDB/GDB) or a built-in lightweight backend. Its MLang-specific job is
**pretty-printing**: it formats MLang values the way the programmer thinks of them
rather than as raw memory:

- `String` shows its text, not a struct of pointer/length/capacity.
- `List<T>`/`Map<K,V>` show their elements, not their internal buffers.
- `T?` shows the value or `null`, not an option's tag-and-payload.
- `Rc<T>`/`Weak<T>` show the pointee and the strong/weak counts.
- coroutine frames show the logical call (the `async` function and its `await`
  point), not the lowered state machine (Chapter 16).

These are driven by the same reflection/metadata tables (Chapter 19) so they stay
correct as types evolve.

## 25.4 Async and concurrency debugging

Debugging coroutines is notoriously hard because the OS stack does not reflect the
logical `await` chain. The adapter reconstructs **logical stacks**: it walks the
chain of suspended frames (parent task -> awaited task) from the runtime's task
metadata, so a developer sees "handleRequest awaiting fetchUser awaiting
db.query", not three unrelated worker-thread stacks. It can also list all live
tasks and their states (running/suspended/cancelled), which is the concurrency
equivalent of a thread list.

## 25.5 Breakpoints and stepping

Standard source breakpoints, conditional breakpoints (a condition compiled and
evaluated by the debugger), logpoints, and data/watchpoints (where the hardware
supports them). Stepping is source-line based; "step over" an `await` resumes when
the awaited task completes, presenting the logical rather than the scheduler-level
behavior.

## 25.6 Integration

The VS Code extension (Chapter 24) bundles the DAP adapter as its debug provider;
any DAP editor can use the same adapter. `mbuild` produces debug builds with the
right flags by default in the `dev` profile (Chapter 22), so "F5 to debug" works
without configuration.
