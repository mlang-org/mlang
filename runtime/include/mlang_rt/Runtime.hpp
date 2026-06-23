#ifndef MLANG_RT_RUNTIME_HPP
#define MLANG_RT_RUNTIME_HPP

#include <cstddef>
#include <cstdint>

// The mlang-runtime public ABI. These are the intrinsics the compiler lowers to
// (docs/design/12-runtime.md, 29-abi.md). They are extern "C" so the calling
// convention is stable and the symbols are unmangled. This is a minimal,
// correct baseline; the size-classed allocator and work-stealing scheduler
// described in the design are layered on top of these entry points.

extern "C" {

// --- Object header shared by reference-counted allocations -----------------
typedef struct MlangObjectHeader {
    uint64_t strong;   // strong reference count
    uint64_t weak;     // weak reference count
    const void* typeInfo;
} MlangObjectHeader;

// --- Allocation ------------------------------------------------------------
void* mlang_alloc(size_t size, size_t align);
void mlang_free(void* ptr);

// Allocates an object with a leading MlangObjectHeader (strong count = 1).
void* mlang_alloc_object(size_t payloadSize, size_t align, const void* typeInfo);

// --- Automatic reference counting -----------------------------------------
void mlang_retain(void* obj);
// Decrements the strong count; runs the destructor and frees when it hits zero.
void mlang_release(void* obj, void (*destructor)(void*));

// --- Panics ----------------------------------------------------------------
[[noreturn]] void mlang_panic(const char* message, const char* file, uint32_t line);
void mlang_assert(int condition, const char* message, const char* file, uint32_t line);

// --- Startup ---------------------------------------------------------------
// Initializes the runtime, runs the user entry point, and shuts down. The
// compiler-generated _mlang_start calls this.
int mlang_runtime_main(int argc, char** argv, int (*userMain)(void));

// --- Introspection (for tests/tools) --------------------------------------
uint64_t mlang_rt_live_allocations(void);

} // extern "C"

#endif
