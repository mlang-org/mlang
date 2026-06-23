#include "mlang/ir/MirBuilder.hpp"

namespace mlang::ir {

const types::Type* MirBuilder::lowerType(const ast::TypeRef* ref) const {
    if (!ref) {
        return types_.getVoid();
    }
    switch (ref->form) {
    case ast::TypeRef::Form::Array:
        return types_.array(lowerType(ref->elem));
    case ast::TypeRef::Form::Map:
        return types_.map(lowerType(ref->elem), lowerType(ref->value));
    case ast::TypeRef::Form::Func: {
        std::vector<const types::Type*> params;
        params.reserve(ref->params.size());
        for (const auto* p : ref->params) {
            params.push_back(lowerType(p));
        }
        return types_.function(std::move(params), lowerType(ref->ret), ref->asyncFn);
    }
    case ast::TypeRef::Form::Named: {
        const types::Type* base = nullptr;
        if (ref->name == "int" || ref->name == "Int") base = types_.getInt();
        else if (ref->name == "float" || ref->name == "Float") base = types_.getFloat();
        else if (ref->name == "bool" || ref->name == "Bool") base = types_.primitive(types::TypeKind::Bool);
        else if (ref->name == "String") base = types_.getString();
        else if (ref->name == "void") base = types_.getVoid();
        else {
            std::vector<const types::Type*> args;
            for (const auto* a : ref->args) {
                args.push_back(lowerType(a));
            }
            base = types_.named(ref->name, std::move(args));
        }
        return ref->nullable ? types_.nullable(base) : base;
    }
    }
    return types_.getError();
}

void MirBuilder::lowerFunction(const ast::FnDecl& fn, Module& out) {
    Function& f = out.addFunction(fn.name);
    f.isAsync = fn.modifiers.isAsync;
    for (const auto* p : fn.params) {
        f.paramTypes.push_back(lowerType(p->type));
    }
    f.returnType = lowerType(fn.returnType);

    Block& entry = f.addBlock("bb0");
    // Statement/expression lowering to SSA is the next increment; for now the
    // entry block returns, giving a verifiable, well-formed skeleton.
    entry.terminator.kind = TermKind::Ret;
}

void MirBuilder::lowerMembers(const std::vector<ast::Decl*>& members, Module& out) {
    for (const ast::Decl* d : members) {
        if (d->kind == ast::NodeKind::FnDecl) {
            lowerFunction(*static_cast<const ast::FnDecl*>(d), out);
        }
    }
}

Module MirBuilder::build(const ast::Module& module) {
    Module out;
    out.name = module.fileName;
    for (const ast::Decl* d : module.declarations) {
        switch (d->kind) {
        case ast::NodeKind::FnDecl:
            lowerFunction(*static_cast<const ast::FnDecl*>(d), out);
            break;
        case ast::NodeKind::ClassDecl:
            lowerMembers(static_cast<const ast::ClassDecl*>(d)->members, out);
            break;
        case ast::NodeKind::EnumDecl:
            lowerMembers(static_cast<const ast::EnumDecl*>(d)->members, out);
            break;
        case ast::NodeKind::ExtensionDecl:
            lowerMembers(static_cast<const ast::ExtensionDecl*>(d)->members, out);
            break;
        default:
            break;
        }
    }
    return out;
}

} // namespace mlang::ir
