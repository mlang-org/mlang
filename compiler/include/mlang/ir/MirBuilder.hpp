#ifndef MLANG_IR_MIRBUILDER_HPP
#define MLANG_IR_MIRBUILDER_HPP

#include "mlang/ast/Ast.hpp"
#include "mlang/ir/Mir.hpp"
#include "mlang/types/Type.hpp"

namespace mlang::ir {

// Lowers a typed AST module to MIR. This first increment emits a function
// skeleton (signature + entry block + return) for each function so the IR
// pipeline and dumps are exercised end to end; statement/expression lowering to
// SSA is layered on next (docs/design/09-ir.md).
class MirBuilder {
public:
    explicit MirBuilder(types::TypeContext& types) : types_(types) {}

    Module build(const ast::Module& module);

private:
    void lowerFunction(const ast::FnDecl& fn, Module& out);
    void lowerMembers(const std::vector<ast::Decl*>& members, Module& out);
    const types::Type* lowerType(const ast::TypeRef* ref) const;

    types::TypeContext& types_;
};

} // namespace mlang::ir

#endif
