#include "mlang/types/Type.hpp"

namespace mlang::types {

namespace {
const char* primitiveName(TypeKind k) {
    switch (k) {
    case TypeKind::Void: return "void";
    case TypeKind::Bool: return "bool";
    case TypeKind::Char: return "char";
    case TypeKind::Int8: return "i8";
    case TypeKind::Int16: return "i16";
    case TypeKind::Int32: return "i32";
    case TypeKind::Int64: return "int";
    case TypeKind::UInt8: return "u8";
    case TypeKind::UInt16: return "u16";
    case TypeKind::UInt32: return "u32";
    case TypeKind::UInt64: return "uint";
    case TypeKind::Float32: return "f32";
    case TypeKind::Float64: return "float";
    case TypeKind::String: return "String";
    case TypeKind::Nothing: return "Nothing";
    case TypeKind::Any: return "Any";
    case TypeKind::Error: return "<error>";
    default: return "?";
    }
}
} // namespace

std::string Type::toString() const {
    switch (kind) {
    case TypeKind::Nullable:
        return (element ? element->toString() : "?") + "?";
    case TypeKind::Array:
        return "[" + (element ? element->toString() : "?") + "]";
    case TypeKind::Map:
        return "[" + (element ? element->toString() : "?") + ": " +
               (value ? value->toString() : "?") + "]";
    case TypeKind::Function: {
        std::string s = asyncFn ? "async (" : "(";
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) s += ", ";
            s += args[i]->toString();
        }
        s += ") -> ";
        s += result ? result->toString() : "void";
        return s;
    }
    case TypeKind::Named:
    case TypeKind::TypeVar: {
        std::string s = name;
        if (!args.empty()) {
            s += "<";
            for (std::size_t i = 0; i < args.size(); ++i) {
                if (i) s += ", ";
                s += args[i]->toString();
            }
            s += ">";
        }
        return s;
    }
    default:
        return primitiveName(kind);
    }
}

TypeContext::TypeContext() {
    for (int k = 0; k <= static_cast<int>(TypeKind::String); ++k) {
        auto t = std::make_unique<Type>();
        t->kind = static_cast<TypeKind>(k);
        primitives_[k] = t.get();
        storage_.push_back(std::move(t));
    }
}

const Type* TypeContext::primitive(TypeKind kind) const {
    const int idx = static_cast<int>(kind);
    if (idx >= 0 && idx <= static_cast<int>(TypeKind::String)) {
        return primitives_[idx];
    }
    // Nothing / Any / Error are interned lazily via named() fallthrough.
    static Type nothingT{TypeKind::Nothing};
    static Type anyT{TypeKind::Any};
    static Type errorT{TypeKind::Error};
    switch (kind) {
    case TypeKind::Nothing: return &nothingT;
    case TypeKind::Any: return &anyT;
    default: return &errorT;
    }
}

const Type* TypeContext::intern(Type&& t) {
    const std::string key = t.toString();
    if (auto it = cache_.find(key); it != cache_.end()) {
        return it->second;
    }
    auto owned = std::make_unique<Type>(std::move(t));
    const Type* ptr = owned.get();
    storage_.push_back(std::move(owned));
    cache_.emplace(key, ptr);
    return ptr;
}

const Type* TypeContext::nullable(const Type* inner) {
    if (inner && inner->kind == TypeKind::Nullable) {
        return inner;
    }
    Type t{TypeKind::Nullable};
    t.element = inner;
    return intern(std::move(t));
}

const Type* TypeContext::array(const Type* element) {
    Type t{TypeKind::Array};
    t.element = element;
    return intern(std::move(t));
}

const Type* TypeContext::map(const Type* key, const Type* value) {
    Type t{TypeKind::Map};
    t.element = key;
    t.value = value;
    return intern(std::move(t));
}

const Type* TypeContext::function(std::vector<const Type*> params, const Type* result,
                                  bool async) {
    Type t{TypeKind::Function};
    t.args = std::move(params);
    t.result = result;
    t.asyncFn = async;
    return intern(std::move(t));
}

const Type* TypeContext::named(const std::string& name, std::vector<const Type*> args) {
    Type t{TypeKind::Named};
    t.name = name;
    t.args = std::move(args);
    return intern(std::move(t));
}

bool TypeContext::isAssignable(const Type* from, const Type* to) const {
    if (from == to) {
        return true;
    }
    if (!from || !to) {
        return false;
    }
    if (from->kind == TypeKind::Error || to->kind == TypeKind::Error) {
        return true; // suppress cascades
    }
    if (from->kind == TypeKind::Nothing) {
        return true; // bottom is assignable to anything
    }
    if (to->kind == TypeKind::Any) {
        return true; // everything is an Any
    }
    if (to->kind == TypeKind::Nullable) {
        // T is assignable to T?
        return isAssignable(from, to->element) ||
               (from->kind == TypeKind::Nullable && isAssignable(from->element, to->element));
    }
    return false;
}

} // namespace mlang::types
