#ifndef MLANG_SEMA_SEMA_HPP
#define MLANG_SEMA_SEMA_HPP

#include "mlang/ast/Ast.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/types/Type.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace mlang::sema {

// A resolved program symbol. The full symbol table grows from here as name
// resolution, type checking, and ownership analysis are implemented
// (docs/design/08-semantic-analysis.md).
struct Symbol {
    enum class Kind { Function, Field, Class, Struct, Interface, Enum, Extension };
    Kind kind;
    std::string name;
    support::SourceRange declaredAt;
};

// The semantic analyzer facade. This first increment performs name collection
// and duplicate-declaration detection; type checking and ownership analysis are
// layered on top in later phases.
class Sema {
public:
    Sema(support::DiagnosticEngine& diags, types::TypeContext& types)
        : diags_(diags), types_(types) {}

    void analyze(const ast::Module& module);

    [[nodiscard]] const std::vector<Symbol>& globals() const { return globals_; }

private:
    void collectScope(const std::vector<ast::Decl*>& decls,
                      std::unordered_map<std::string, support::SourceRange>& seen,
                      bool isGlobal);

    support::DiagnosticEngine& diags_;
    types::TypeContext& types_;
    std::vector<Symbol> globals_;
};

} // namespace mlang::sema

#endif
