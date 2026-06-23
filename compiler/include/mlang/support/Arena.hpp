#ifndef MLANG_SUPPORT_ARENA_HPP
#define MLANG_SUPPORT_ARENA_HPP

#include <memory>
#include <utility>
#include <vector>

namespace mlang::support {

// Owns a set of heterogeneous objects with a shared lifetime. Objects are
// constructed in place and destroyed together when the arena dies. Returned
// pointers stay valid for the arena's lifetime.
class Arena {
public:
    Arena() = default;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;
    ~Arena() = default;

    template <typename T, typename... Args>
    T* make(Args&&... args) {
        auto owned = std::make_unique<Holder<T>>(std::forward<Args>(args)...);
        T* ptr = &owned->value;
        objects_.push_back(std::move(owned));
        return ptr;
    }

    [[nodiscard]] std::size_t size() const noexcept { return objects_.size(); }

private:
    struct Erased {
        virtual ~Erased() = default;
    };

    template <typename T>
    struct Holder final : Erased {
        T value;
        template <typename... Args>
        explicit Holder(Args&&... args) : value(std::forward<Args>(args)...) {}
    };

    std::vector<std::unique_ptr<Erased>> objects_;
};

} // namespace mlang::support

#endif
