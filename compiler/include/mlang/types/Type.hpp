#ifndef MLANG_TYPES_TYPE_HPP
#define MLANG_TYPES_TYPE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mlang::types {

enum class TypeKind {
    // Primitives
    Void, Bool, Char,
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64,
    String,
    // Composite
    Nullable,   // T?
    Array,      // [T]
    Map,        // [K: V]
    Function,   // (params) -> ret
    Named,      // class/struct/interface/enum (nominal), with type args
    TypeVar,    // a generic type parameter
    Nothing,    // bottom type
    Any,        // top type
    Error,      // assigned on failure to suppress cascades
};

// A semantic type. Types are interned by TypeContext, so canonical Type
// pointers compare by identity. See docs/design/07-type-system.md.
struct Type {
    Type() = default;
    explicit Type(TypeKind k) : kind(k) {}

    TypeKind kind = TypeKind::Error;
    std::string name;                 // Named/TypeVar
    std::vector<const Type*> args;     // Named type arguments / Function params
    const Type* element = nullptr;     // Nullable inner, Array element, Map key
    const Type* value = nullptr;       // Map value
    const Type* result = nullptr;      // Function result
    bool asyncFn = false;

    [[nodiscard]] bool isNullable() const { return kind == TypeKind::Nullable; }
    [[nodiscard]] bool isPrimitive() const {
        return kind >= TypeKind::Void && kind <= TypeKind::String;
    }
    [[nodiscard]] std::string toString() const;
};

// Owns and interns all types for one compilation.
class TypeContext {
public:
    TypeContext();

    const Type* primitive(TypeKind kind) const;
    const Type* getVoid() const { return primitive(TypeKind::Void); }
    const Type* getBool() const { return primitive(TypeKind::Bool); }
    const Type* getInt() const { return primitive(TypeKind::Int64); }
    const Type* getFloat() const { return primitive(TypeKind::Float64); }
    const Type* getString() const { return primitive(TypeKind::String); }
    const Type* getError() const { return primitive(TypeKind::Error); }
    const Type* getNothing() const { return primitive(TypeKind::Nothing); }
    const Type* getAny() const { return primitive(TypeKind::Any); }

    const Type* nullable(const Type* inner);
    const Type* array(const Type* element);
    const Type* map(const Type* key, const Type* value);
    const Type* function(std::vector<const Type*> params, const Type* result, bool async = false);
    const Type* named(const std::string& name, std::vector<const Type*> args = {});

    // Structural subtyping check (nominal types compare by identity for now).
    bool isAssignable(const Type* from, const Type* to) const;

private:
    const Type* intern(Type&& t);

    std::vector<std::unique_ptr<Type>> storage_;
    std::unordered_map<std::string, const Type*> cache_;
    const Type* primitives_[static_cast<int>(TypeKind::String) + 1] = {};
};

} // namespace mlang::types

#endif
