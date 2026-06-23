# 12. Runtime Architecture

`mlang-runtime` is the small native library every MLang program links against. It
is written in C++23 with a few assembly stubs (context switching, unwinding
helpers). The design goal is *minimal*: the runtime does only what cannot be done
in MLang or in generated code.

## 12.1 What the runtime contains

```
mlang-runtime
├── alloc/        memory allocator (size-classed, thread-caching)
├── arc/          retain/release/drop intrinsics, weak-ref table, cycle collector
├── sched/        work-stealing coroutine scheduler + timers + I/O reactor
├── task/         Task/Future state, channels, sync primitives
├── exn/          panic, unwinding glue, backtrace capture
├── meta/         runtime type info tables consumed by reflection (Chapter 19)
├── intrinsics/   memcpy/memset wrappers, checked arithmetic helpers, string ops
└── start/        _mlang_start, argv handling, environment, shutdown
```

What is **not** here: collections, formatting, I/O abstractions, JSON, HTTP, and
the rest of the standard library; those are MLang code (Chapter 20). The runtime
is the floor, not the building.

## 12.2 Startup and shutdown

`_mlang_start` (the real entry point) initializes the allocator, creates the
scheduler with one worker per hardware thread, installs signal/exception handlers,
constructs the root coroutine that calls user `main`, runs the scheduler until the
root completes and all structured-concurrency children have joined, runs any
process-level destructors, flushes buffered I/O, and returns `main`'s exit code.
Because shutdown joins outstanding tasks, a well-structured program leaks nothing
and truncates nothing on exit.

## 12.3 The allocator

A size-classed, thread-caching allocator (in the family of tcmalloc/mimalloc):

- **Thread-local caches** for small objects: allocation is a pop from a free
  list, no lock, no atomics on the fast path.
- **Size classes** to bound fragmentation; large allocations go straight to the
  OS via `mmap`/`VirtualAlloc`.
- **Per-class spans** with metadata kept out of band so hot allocation paths do
  not pollute the cache with bookkeeping.

Because escape analysis (Chapter 10) keeps many objects on the stack and ARC
frees deterministically, allocator pressure is far lower than in a GC'd runtime,
and there is no heap-walking collector competing for memory bandwidth.

## 12.4 The scheduler

A work-stealing M:N scheduler maps many coroutines onto a few OS threads:

- Each worker has a local run queue (LIFO for cache locality on resume) and steals
  from others' queues (FIFO) when idle.
- Blocking I/O is never allowed to block a worker: I/O goes through the reactor
  (`epoll`/`kqueue`/IOCP), which parks the coroutine and reschedules it on
  completion.
- Timers are a hierarchical timing wheel.
- The scheduler is the substrate for `async`/`await`, `launch`, channels, and
  structured concurrency (Chapters 15 and 16).

## 12.5 Intrinsics and the compiler/runtime contract

The compiler lowers certain operations to calls into the runtime with a stable
ABI: `mlang_alloc`, `mlang_free`, `mlang_retain`, `mlang_release`,
`mlang_weak_load`, `mlang_panic`, `mlang_task_spawn`, `mlang_chan_send`, and so
on. Many are tiny enough to be inlined at `-O2` (the compiler has their bodies as
MIR), so the "call into the runtime" is often not a real call. This contract is
versioned and documented in [Chapter 29](29-abi.md).

## 12.6 Configurability

The runtime exposes compile-time and start-time knobs: number of scheduler
workers, allocator arena sizes, whether the optional cycle collector is enabled
(Chapter 13), stack sizes for coroutines, and whether backtraces are captured on
panic. A freestanding/embedded profile strips the scheduler and I/O reactor for
environments without an OS, leaving allocator + ARC + panic. The runtime is thus
usable from a server down to a microcontroller-class target.
