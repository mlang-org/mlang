#include "mlang/ast/Ast.hpp"

#include <ostream>
#include <string>

namespace mlang::ast {

namespace {

class Printer {
public:
    explicit Printer(std::ostream& os) : os_(os) {}

    void module(const Module& m) {
        line("Module \"" + m.fileName + "\"");
        Indent g(*this);
        if (m.namespaceDecl) {
            decl(m.namespaceDecl);
        }
        for (const auto* imp : m.imports) {
            decl(imp);
        }
        for (const auto* d : m.declarations) {
            decl(d);
        }
    }

private:
    struct Indent {
        Printer& p;
        explicit Indent(Printer& pr) : p(pr) { ++p.depth_; }
        ~Indent() { --p.depth_; }
    };

    void line(const std::string& text) {
        for (int i = 0; i < depth_; ++i) {
            os_ << "  ";
        }
        os_ << text << '\n';
    }

    static std::string typeStr(const TypeRef* t) {
        if (!t) {
            return "<infer>";
        }
        std::string s;
        switch (t->form) {
        case TypeRef::Form::Named:
            s = t->name;
            if (!t->args.empty()) {
                s += "<";
                for (std::size_t i = 0; i < t->args.size(); ++i) {
                    if (i) s += ", ";
                    s += typeStr(t->args[i]);
                }
                s += ">";
            }
            break;
        case TypeRef::Form::Array:
            s = "[" + typeStr(t->elem) + "]";
            break;
        case TypeRef::Form::Map:
            s = "[" + typeStr(t->elem) + ": " + typeStr(t->value) + "]";
            break;
        case TypeRef::Form::Func: {
            s = "(";
            for (std::size_t i = 0; i < t->params.size(); ++i) {
                if (i) s += ", ";
                s += typeStr(t->params[i]);
            }
            s += ") -> " + typeStr(t->ret);
            break;
        }
        }
        if (t->nullable) {
            s += "?";
        }
        return s;
    }

    static std::string mods(const Modifiers& m) {
        std::string s;
        auto add = [&](bool b, const char* name) {
            if (b) { s += s.empty() ? "" : " "; s += name; }
        };
        add(m.isPublic, "public");
        add(m.isPrivate, "private");
        add(m.isProtected, "protected");
        add(m.isInternal, "internal");
        add(m.isStatic, "static");
        add(m.isAbstract, "abstract");
        add(m.isVirtual, "virtual");
        add(m.isOverride, "override");
        add(m.isAsync, "async");
        add(m.isReadonly, "readonly");
        add(m.isSealed, "sealed");
        add(m.isUnsafe, "unsafe");
        return s.empty() ? s : "[" + s + "] ";
    }

    void params(const std::vector<Param*>& ps) {
        for (const auto* p : ps) {
            line("Param " + p->name + ": " + typeStr(p->type));
            if (p->defaultValue) {
                Indent g(*this);
                expr(p->defaultValue);
            }
        }
    }

    void decl(const Decl* d) {
        for (const auto& a : d->annotations) {
            line("@" + a.name);
        }
        switch (d->kind) {
        case NodeKind::NamespaceDecl:
            line("Namespace " + static_cast<const NamespaceDecl*>(d)->name);
            break;
        case NodeKind::ImportDecl: {
            const auto* i = static_cast<const ImportDecl*>(d);
            std::string s = "Import " + i->path;
            if (i->wildcard) s += ".*";
            if (!i->alias.empty()) s += " as " + i->alias;
            if (!i->items.empty()) {
                s += " {";
                for (std::size_t k = 0; k < i->items.size(); ++k) {
                    if (k) s += ", ";
                    s += i->items[k];
                }
                s += "}";
            }
            line(s);
            break;
        }
        case NodeKind::FnDecl: {
            const auto* f = static_cast<const FnDecl*>(d);
            line(mods(f->modifiers) + "Fn " + f->name + " -> " + typeStr(f->returnType));
            Indent g(*this);
            params(f->params);
            if (f->body) {
                stmt(f->body);
            }
            break;
        }
        case NodeKind::FieldDecl: {
            const auto* f = static_cast<const FieldDecl*>(d);
            line(mods(f->modifiers) + (f->isVar ? "Var " : "Let ") + f->name + ": " +
                 typeStr(f->type));
            if (f->init) {
                Indent g(*this);
                expr(f->init);
            }
            break;
        }
        case NodeKind::ClassDecl: {
            const auto* c = static_cast<const ClassDecl*>(d);
            const char* shape = c->shape == ClassDecl::Shape::Struct ? "Struct"
                              : c->shape == ClassDecl::Shape::Interface ? "Interface"
                                                                        : "Class";
            std::string s = mods(c->modifiers) + shape + " " + c->name;
            if (c->baseClass) s += " ext " + typeStr(c->baseClass);
            if (!c->interfaces.empty()) {
                s += " impl ";
                for (std::size_t k = 0; k < c->interfaces.size(); ++k) {
                    if (k) s += ", ";
                    s += typeStr(c->interfaces[k]);
                }
            }
            line(s);
            Indent g(*this);
            for (const auto* m : c->members) {
                decl(m);
            }
            break;
        }
        case NodeKind::EnumDecl: {
            const auto* e = static_cast<const EnumDecl*>(d);
            line(mods(e->modifiers) + "Enum " + e->name);
            Indent g(*this);
            for (const auto& cs : e->cases) {
                line("Case " + cs.name);
            }
            for (const auto* m : e->members) {
                decl(m);
            }
            break;
        }
        case NodeKind::ExtensionDecl: {
            const auto* e = static_cast<const ExtensionDecl*>(d);
            line("Extension " + typeStr(e->target));
            Indent g(*this);
            for (const auto* m : e->members) {
                decl(m);
            }
            break;
        }
        case NodeKind::ConstructorDecl: {
            const auto* c = static_cast<const ConstructorDecl*>(d);
            line("Constructor");
            Indent g(*this);
            params(c->params);
            if (c->body) stmt(c->body);
            break;
        }
        case NodeKind::DestructorDecl: {
            const auto* c = static_cast<const DestructorDecl*>(d);
            line("Destructor");
            Indent g(*this);
            if (c->body) stmt(c->body);
            break;
        }
        default:
            line("<decl?>");
            break;
        }
    }

    void stmt(const Stmt* s) {
        switch (s->kind) {
        case NodeKind::BlockStmt: {
            line("Block");
            Indent g(*this);
            for (const auto* st : static_cast<const BlockStmt*>(s)->statements) {
                stmt(st);
            }
            break;
        }
        case NodeKind::VarDeclStmt: {
            const auto* v = static_cast<const VarDeclStmt*>(s);
            line((v->isConst ? "Const " : v->isVar ? "Var " : "Let ") + v->name + ": " +
                 typeStr(v->type));
            if (v->init) {
                Indent g(*this);
                expr(v->init);
            }
            break;
        }
        case NodeKind::ExprStmt:
            line("ExprStmt");
            { Indent g(*this); expr(static_cast<const ExprStmt*>(s)->expr); }
            break;
        case NodeKind::IfStmt: {
            const auto* i = static_cast<const IfStmt*>(s);
            line("If");
            Indent g(*this);
            line("cond:"); { Indent h(*this); expr(i->cond); }
            line("then:"); { Indent h(*this); stmt(i->thenBranch); }
            if (i->elseBranch) {
                line("else:"); { Indent h(*this); stmt(i->elseBranch); }
            }
            break;
        }
        case NodeKind::WhileStmt: {
            const auto* w = static_cast<const WhileStmt*>(s);
            line("While");
            Indent g(*this);
            expr(w->cond);
            stmt(w->body);
            break;
        }
        case NodeKind::DoWhileStmt: {
            const auto* w = static_cast<const DoWhileStmt*>(s);
            line("DoWhile");
            Indent g(*this);
            stmt(w->body);
            expr(w->cond);
            break;
        }
        case NodeKind::ForStmt: {
            const auto* f = static_cast<const ForStmt*>(s);
            line("For " + f->varName + " in");
            Indent g(*this);
            expr(f->iterable);
            stmt(f->body);
            break;
        }
        case NodeKind::ReturnStmt: {
            line("Return");
            const auto* r = static_cast<const ReturnStmt*>(s);
            if (r->value) { Indent g(*this); expr(r->value); }
            break;
        }
        case NodeKind::BreakStmt: line("Break"); break;
        case NodeKind::ContinueStmt: line("Continue"); break;
        case NodeKind::ThrowStmt:
            line("Throw");
            { Indent g(*this); expr(static_cast<const ThrowStmt*>(s)->value); }
            break;
        case NodeKind::TryStmt: {
            const auto* t = static_cast<const TryStmt*>(s);
            line("Try");
            Indent g(*this);
            stmt(t->body);
            for (const auto& c : t->catches) {
                line("Catch " + c.name + ": " + typeStr(c.type));
                Indent h(*this);
                if (c.body) stmt(c.body);
            }
            if (t->finallyBody) {
                line("Finally");
                Indent h(*this);
                stmt(t->finallyBody);
            }
            break;
        }
        case NodeKind::MatchStmt: {
            const auto* m = static_cast<const MatchStmt*>(s);
            line("Match");
            Indent g(*this);
            expr(m->subject);
            for (const auto& arm : m->arms) {
                line(arm.isDefault ? "default =>" : ("case " + arm.text + " =>"));
                Indent h(*this);
                if (arm.body) stmt(arm.body);
            }
            break;
        }
        case NodeKind::LaunchStmt:
            line("Launch");
            { Indent g(*this); if (static_cast<const LaunchStmt*>(s)->body) stmt(static_cast<const LaunchStmt*>(s)->body); }
            break;
        default:
            line("<stmt?>");
            break;
        }
    }

    void expr(const Expr* e) {
        if (!e) { line("<null-expr>"); return; }
        switch (e->kind) {
        case NodeKind::LiteralExpr:
            line("Literal " + std::string(lexer::tokenKindName(
                     static_cast<const LiteralExpr*>(e)->literalKind)) + " " +
                 static_cast<const LiteralExpr*>(e)->text);
            break;
        case NodeKind::IdentExpr:
            line("Ident " + static_cast<const IdentExpr*>(e)->name);
            break;
        case NodeKind::ThisExpr: line("This"); break;
        case NodeKind::SuperExpr: line("Super"); break;
        case NodeKind::UnaryExpr: {
            const auto* u = static_cast<const UnaryExpr*>(e);
            line("Unary " + std::string(lexer::tokenKindName(u->op)));
            Indent g(*this); expr(u->operand);
            break;
        }
        case NodeKind::PostfixExpr: {
            const auto* u = static_cast<const PostfixExpr*>(e);
            line("Postfix " + std::string(lexer::tokenKindName(u->op)));
            Indent g(*this); expr(u->operand);
            break;
        }
        case NodeKind::BinaryExpr: {
            const auto* b = static_cast<const BinaryExpr*>(e);
            line("Binary " + std::string(lexer::tokenKindName(b->op)));
            Indent g(*this); expr(b->lhs); expr(b->rhs);
            break;
        }
        case NodeKind::AssignExpr: {
            const auto* a = static_cast<const AssignExpr*>(e);
            line("Assign " + std::string(lexer::tokenKindName(a->op)));
            Indent g(*this); expr(a->target); expr(a->value);
            break;
        }
        case NodeKind::TernaryExpr: {
            const auto* t = static_cast<const TernaryExpr*>(e);
            line("Ternary");
            Indent g(*this); expr(t->cond); expr(t->thenExpr); expr(t->elseExpr);
            break;
        }
        case NodeKind::CallExpr: {
            const auto* c = static_cast<const CallExpr*>(e);
            line("Call");
            Indent g(*this);
            line("callee:"); { Indent h(*this); expr(c->callee); }
            for (const auto& a : c->args) {
                line("arg" + (a.name.empty() ? std::string() : " " + a.name) + ":");
                Indent h(*this); expr(a.value);
            }
            break;
        }
        case NodeKind::MemberExpr: {
            const auto* m = static_cast<const MemberExpr*>(e);
            line(std::string(m->safe ? "SafeMember ." : "Member .") + m->name);
            Indent g(*this); expr(m->object);
            break;
        }
        case NodeKind::IndexExpr: {
            const auto* i = static_cast<const IndexExpr*>(e);
            line("Index");
            Indent g(*this); expr(i->object); expr(i->index);
            break;
        }
        case NodeKind::LambdaExpr: {
            const auto* l = static_cast<const LambdaExpr*>(e);
            line("Lambda");
            Indent g(*this);
            params(l->params);
            if (l->exprBody) expr(l->exprBody);
            if (l->blockBody) stmt(l->blockBody);
            break;
        }
        case NodeKind::ArrayLiteralExpr: {
            const auto* a = static_cast<const ArrayLiteralExpr*>(e);
            line("ArrayLiteral");
            Indent g(*this);
            for (const auto* el : a->elements) expr(el);
            break;
        }
        case NodeKind::CastExpr: {
            const auto* c = static_cast<const CastExpr*>(e);
            line("Cast as " + typeStr(c->type));
            Indent g(*this); expr(c->value);
            break;
        }
        case NodeKind::IsExpr: {
            const auto* c = static_cast<const IsExpr*>(e);
            line("Is " + typeStr(c->type));
            Indent g(*this); expr(c->value);
            break;
        }
        case NodeKind::NewExpr: {
            const auto* n = static_cast<const NewExpr*>(e);
            line("New " + typeStr(n->type));
            Indent g(*this);
            for (const auto& a : n->args) expr(a.value);
            break;
        }
        case NodeKind::AwaitExpr: {
            line("Await");
            Indent g(*this); expr(static_cast<const AwaitExpr*>(e)->operand);
            break;
        }
        case NodeKind::TryExpr: {
            line("Try?");
            Indent g(*this); expr(static_cast<const TryExpr*>(e)->operand);
            break;
        }
        case NodeKind::ScopeExpr: {
            line("Scope");
            Indent g(*this);
            if (static_cast<const ScopeExpr*>(e)->body) stmt(static_cast<const ScopeExpr*>(e)->body);
            break;
        }
        case NodeKind::LaunchExpr: {
            const auto* l = static_cast<const LaunchExpr*>(e);
            line("Launch");
            Indent g(*this);
            if (l->operand) expr(l->operand);
            if (l->block) stmt(l->block);
            break;
        }
        case NodeKind::ErrorExpr: line("<error-expr>"); break;
        default: line("<expr?>"); break;
        }
    }

    std::ostream& os_;
    int depth_ = 0;
};

} // namespace

void printModule(const Module& module, std::ostream& os) {
    Printer(os).module(module);
}

} // namespace mlang::ast
