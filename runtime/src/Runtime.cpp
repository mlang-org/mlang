#include "mlang_rt/Runtime.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// Minimal but correct reference implementation of the mlang-runtime ABI. The
// production allocator (thread-caching, size-classed) and scheduler replace the
// bodies here without changing the interface (docs/design/12-runtime.md).

namespace {
std::atomic<uint64_t> g_liveAllocations{0};

constexpr std::size_t kHeaderSize = sizeof(MlangObjectHeader);
} // namespace

extern "C" {

void* mlang_alloc(size_t size, size_t align) {
    if (size == 0) {
        size = 1;
    }
    if (align < alignof(std::max_align_t)) {
        align = alignof(std::max_align_t);
    }
    // Round size up to a multiple of alignment for aligned allocation.
    const size_t rounded = (size + align - 1) & ~(align - 1);
    void* ptr = ::operator new(rounded, std::align_val_t{align}, std::nothrow);
    if (ptr) {
        g_liveAllocations.fetch_add(1, std::memory_order_relaxed);
    }
    return ptr;
}

void mlang_free(void* ptr) {
    if (ptr) {
        g_liveAllocations.fetch_sub(1, std::memory_order_relaxed);
        ::operator delete(ptr, std::nothrow);
    }
}

void* mlang_alloc_object(size_t payloadSize, size_t align, const void* typeInfo) {
    if (align < alignof(MlangObjectHeader)) {
        align = alignof(MlangObjectHeader);
    }
    void* raw = mlang_alloc(kHeaderSize + payloadSize, align);
    if (!raw) {
        return nullptr;
    }
    auto* header = static_cast<MlangObjectHeader*>(raw);
    header->strong = 1;
    header->weak = 0;
    header->typeInfo = typeInfo;
    return static_cast<char*>(raw) + kHeaderSize;
}

void mlang_retain(void* obj) {
    if (!obj) {
        return;
    }
    auto* header = reinterpret_cast<MlangObjectHeader*>(static_cast<char*>(obj) - kHeaderSize);
    std::atomic_ref<uint64_t> strong(header->strong);
    strong.fetch_add(1, std::memory_order_relaxed);
}

void mlang_release(void* obj, void (*destructor)(void*)) {
    if (!obj) {
        return;
    }
    auto* header = reinterpret_cast<MlangObjectHeader*>(static_cast<char*>(obj) - kHeaderSize);
    std::atomic_ref<uint64_t> strong(header->strong);
    if (strong.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        if (destructor) {
            destructor(obj);
        }
        mlang_free(header);
    }
}

[[noreturn]] void mlang_panic(const char* message, const char* file, uint32_t line) {
    std::fprintf(stderr, "mlang: panic at %s:%u: %s\n", file ? file : "<unknown>", line,
                 message ? message : "");
    std::abort();
}

void mlang_assert(int condition, const char* message, const char* file, uint32_t line) {
    if (!condition) {
        mlang_panic(message ? message : "assertion failed", file, line);
    }
}

int mlang_runtime_main(int /*argc*/, char** /*argv*/, int (*userMain)(void)) {
    // The full version initializes the scheduler, runs the root coroutine, and
    // joins structured-concurrency children before shutdown.
    if (!userMain) {
        return 0;
    }
    const int code = userMain();
    return code;
}

uint64_t mlang_rt_live_allocations(void) {
    return g_liveAllocations.load(std::memory_order_relaxed);
}

} // extern "C"
