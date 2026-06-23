#include "mlang/sema/Sema.hpp"

namespace mlang::sema {

namespace {
const std::string& declName(const ast::Decl* d) {
    static const std::string empty;
    switch (d->kind) {
    case ast::NodeKind::FnDecl: return static_cast<const ast::FnDecl*>(d)->name;
    case ast::NodeKind::FieldDecl: return static_cast<const ast::FieldDecl*>(d)->name;
    case ast::NodeKind::ClassDecl: return static_cast<const ast::ClassDecl*>(d)->name;
    case ast::NodeKind::EnumDecl: return static_cast<const ast::EnumDecl*>(d)->name;
    default: return empty;
    }
}

Symbol::Kind declSymbolKind(const ast::Decl* d) {
    switch (d->kind) {
    case ast::NodeKind::FnDecl: return Symbol::Kind::Function;
    case ast::NodeKind::FieldDecl: return Symbol::Kind::Field;
    case ast::NodeKind::EnumDecl: return Symbol::Kind::Enum;
    case ast::NodeKind::ExtensionDecl: return Symbol::Kind::Extension;
    case ast::NodeKind::ClassDecl: {
        switch (static_cast<const ast::ClassDecl*>(d)->shape) {
        case ast::ClassDecl::Shape::Struct: return Symbol::Kind::Struct;
        case ast::ClassDecl::Shape::Interface: return Symbol::Kind::Interface;
        default: return Symbol::Kind::Class;
        }
    }
    default: return Symbol::Kind::Field;
    }
}
} // namespace

void Sema::collectScope(const std::vector<ast::Decl*>& decls,
                        std::unordered_map<std::string, support::SourceRange>& seen,
                        bool isGlobal) {
    for (const ast::Decl* d : decls) {
        const std::string& name = declName(d);
        if (name.empty()) {
            continue;
        }
        if (auto it = seen.find(name); it != seen.end()) {
            support::Diagnostic diag;
            diag.severity = support::Severity::Error;
            diag.code = "E0200";
            diag.message = "duplicate declaration of '" + name + "' in this scope";
            diag.range = d->range;
            diag.notes.push_back("a declaration with the same name already exists here");
            diags_.report(std::move(diag));
        } else {
            seen.emplace(name, d->range);
            if (isGlobal) {
                globals_.push_back(Symbol{declSymbolKind(d), name, d->range});
            }
        }

        // Recurse into member scopes for nested duplicate detection.
        if (d->kind == ast::NodeKind::ClassDecl) {
            std::unordered_map<std::string, support::SourceRange> members;
            collectScope(static_cast<const ast::ClassDecl*>(d)->members, members, false);
        } else if (d->kind == ast::NodeKind::EnumDecl) {
            std::unordered_map<std::string, support::SourceRange> members;
            collectScope(static_cast<const ast::EnumDecl*>(d)->members, members, false);
        }
    }
}

void Sema::analyze(const ast::Module& module) {
    std::unordered_map<std::string, support::SourceRange> globalSeen;
    collectScope(module.declarations, globalSeen, true);
}

} // namespace mlang::sema
