#ifndef MLANG_AST_AST_HPP
#define MLANG_AST_AST_HPP

#include "mlang/lexer/Token.hpp"
#include "mlang/support/SourceManager.hpp"

#include <string>
#include <vector>

namespace mlang::ast {

using support::SourceRange;

enum class NodeKind {
    // Expressions
    LiteralExpr, IdentExpr, ThisExpr, SuperExpr,
    UnaryExpr, BinaryExpr, AssignExpr, TernaryExpr,
    CallExpr, MemberExpr, IndexExpr, PostfixExpr,
    LambdaExpr, ArrayLiteralExpr, CastExpr, IsExpr, NewExpr, AwaitExpr,
    ScopeExpr, LaunchExpr, TryExpr, ErrorExpr,
    // Statements
    BlockStmt, VarDeclStmt, ExprStmt, IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    ReturnStmt, BreakStmt, ContinueStmt, ThrowStmt, TryStmt, MatchStmt, LaunchStmt,
    // Declarations
    NamespaceDecl, ImportDecl, FnDecl, FieldDecl, ClassDecl, EnumDecl,
    ExtensionDecl, ConstructorDecl, DestructorDecl,
};

struct Node {
    NodeKind kind;
    SourceRange range;
    explicit Node(NodeKind k) : kind(k) {}
    virtual ~Node() = default;
};

// ---------------------------------------------------------------------------
// Types (syntactic type references)
// ---------------------------------------------------------------------------
struct TypeRef {
    enum class Form { Named, Array, Map, Func };
    SourceRange range;
    Form form = Form::Named;
    std::string name;                 // Named: dotted qualified name
    std::vector<TypeRef*> args;       // Named: generic arguments
    bool nullable = false;
    TypeRef* elem = nullptr;          // Array element / Map key
    TypeRef* value = nullptr;         // Map value
    std::vector<TypeRef*> params;     // Func parameters
    TypeRef* ret = nullptr;           // Func return
    bool asyncFn = false;
};

// ---------------------------------------------------------------------------
// Shared building blocks
// ---------------------------------------------------------------------------
struct Annotation {
    std::string name;
    SourceRange range;
};

struct Modifiers {
    bool isPublic = false, isPrivate = false, isProtected = false, isInternal = false;
    bool isStatic = false, isAbstract = false, isVirtual = false, isOverride = false;
    bool isAsync = false, isReadonly = false, isSealed = false, isUnsafe = false;
};

struct Expr : Node {
    using Node::Node;
};
struct Stmt : Node {
    using Node::Node;
};
struct Decl : Node {
    using Node::Node;
    std::vector<Annotation> annotations;
    Modifiers modifiers;
};

struct Param {
    std::string name;
    TypeRef* type = nullptr;
    Expr* defaultValue = nullptr;
    bool readonly = false;
    SourceRange range;
};

struct TypeParam {
    std::string name;
    std::vector<TypeRef*> bounds;
    int variance = 0; // -1 in, 0 invariant, +1 out
};

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------
struct LiteralExpr : Expr {
    lexer::TokenKind literalKind;
    std::string text;
    LiteralExpr() : Expr(NodeKind::LiteralExpr) {}
};
struct IdentExpr : Expr {
    std::string name;
    IdentExpr() : Expr(NodeKind::IdentExpr) {}
};
struct ThisExpr : Expr { ThisExpr() : Expr(NodeKind::ThisExpr) {} };
struct SuperExpr : Expr { SuperExpr() : Expr(NodeKind::SuperExpr) {} };
struct UnaryExpr : Expr {
    lexer::TokenKind op;
    Expr* operand = nullptr;
    UnaryExpr() : Expr(NodeKind::UnaryExpr) {}
};
struct BinaryExpr : Expr {
    lexer::TokenKind op;
    Expr* lhs = nullptr;
    Expr* rhs = nullptr;
    BinaryExpr() : Expr(NodeKind::BinaryExpr) {}
};
struct AssignExpr : Expr {
    lexer::TokenKind op;
    Expr* target = nullptr;
    Expr* value = nullptr;
    AssignExpr() : Expr(NodeKind::AssignExpr) {}
};
struct TernaryExpr : Expr {
    Expr* cond = nullptr;
    Expr* thenExpr = nullptr;
    Expr* elseExpr = nullptr;
    TernaryExpr() : Expr(NodeKind::TernaryExpr) {}
};
struct Argument {
    std::string name; // empty if positional
    Expr* value = nullptr;
};
struct CallExpr : Expr {
    Expr* callee = nullptr;
    std::vector<Argument> args;
    CallExpr() : Expr(NodeKind::CallExpr) {}
};
struct MemberExpr : Expr {
    Expr* object = nullptr;
    std::string name;
    bool safe = false; // ?.
    MemberExpr() : Expr(NodeKind::MemberExpr) {}
};
struct IndexExpr : Expr {
    Expr* object = nullptr;
    Expr* index = nullptr;
    IndexExpr() : Expr(NodeKind::IndexExpr) {}
};
struct PostfixExpr : Expr { // non-null assertion `!`
    lexer::TokenKind op;
    Expr* operand = nullptr;
    PostfixExpr() : Expr(NodeKind::PostfixExpr) {}
};
struct LambdaExpr : Expr {
    std::vector<Param*> params;
    Expr* exprBody = nullptr;
    Stmt* blockBody = nullptr;
    LambdaExpr() : Expr(NodeKind::LambdaExpr) {}
};
struct ArrayLiteralExpr : Expr {
    std::vector<Expr*> elements;
    ArrayLiteralExpr() : Expr(NodeKind::ArrayLiteralExpr) {}
};
struct CastExpr : Expr {
    Expr* value = nullptr;
    TypeRef* type = nullptr;
    CastExpr() : Expr(NodeKind::CastExpr) {}
};
struct IsExpr : Expr {
    Expr* value = nullptr;
    TypeRef* type = nullptr;
    IsExpr() : Expr(NodeKind::IsExpr) {}
};
struct NewExpr : Expr {
    TypeRef* type = nullptr;
    std::vector<Argument> args;
    NewExpr() : Expr(NodeKind::NewExpr) {}
};
struct AwaitExpr : Expr {
    Expr* operand = nullptr;
    AwaitExpr() : Expr(NodeKind::AwaitExpr) {}
};
struct TryExpr : Expr { // postfix `?` error-propagation
    Expr* operand = nullptr;
    TryExpr() : Expr(NodeKind::TryExpr) {}
};
struct ScopeExpr : Expr {
    Stmt* body = nullptr; // a BlockStmt
    ScopeExpr() : Expr(NodeKind::ScopeExpr) {}
};
struct LaunchExpr : Expr {
    Expr* operand = nullptr; // call expression, or null when block is used
    Stmt* block = nullptr;   // a BlockStmt, or null when operand is used
    LaunchExpr() : Expr(NodeKind::LaunchExpr) {}
};
struct ErrorExpr : Expr { ErrorExpr() : Expr(NodeKind::ErrorExpr) {} };

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------
struct BlockStmt : Stmt {
    std::vector<Stmt*> statements;
    BlockStmt() : Stmt(NodeKind::BlockStmt) {}
};
struct VarDeclStmt : Stmt {
    bool isVar = false;
    bool isConst = false;
    std::string name;
    TypeRef* type = nullptr;
    Expr* init = nullptr;
    VarDeclStmt() : Stmt(NodeKind::VarDeclStmt) {}
};
struct ExprStmt : Stmt {
    Expr* expr = nullptr;
    ExprStmt() : Stmt(NodeKind::ExprStmt) {}
};
struct IfStmt : Stmt {
    Expr* cond = nullptr;
    Stmt* thenBranch = nullptr;
    Stmt* elseBranch = nullptr;
    IfStmt() : Stmt(NodeKind::IfStmt) {}
};
struct WhileStmt : Stmt {
    Expr* cond = nullptr;
    Stmt* body = nullptr;
    WhileStmt() : Stmt(NodeKind::WhileStmt) {}
};
struct DoWhileStmt : Stmt {
    Stmt* body = nullptr;
    Expr* cond = nullptr;
    DoWhileStmt() : Stmt(NodeKind::DoWhileStmt) {}
};
struct ForStmt : Stmt {
    std::string varName;
    Expr* iterable = nullptr;
    Stmt* body = nullptr;
    ForStmt() : Stmt(NodeKind::ForStmt) {}
};
struct ReturnStmt : Stmt {
    Expr* value = nullptr;
    ReturnStmt() : Stmt(NodeKind::ReturnStmt) {}
};
struct BreakStmt : Stmt { BreakStmt() : Stmt(NodeKind::BreakStmt) {} };
struct ContinueStmt : Stmt { ContinueStmt() : Stmt(NodeKind::ContinueStmt) {} };
struct ThrowStmt : Stmt {
    Expr* value = nullptr;
    ThrowStmt() : Stmt(NodeKind::ThrowStmt) {}
};
struct CatchClause {
    std::string name;
    TypeRef* type = nullptr;
    BlockStmt* body = nullptr;
};
struct TryStmt : Stmt {
    BlockStmt* body = nullptr;
    std::vector<CatchClause> catches;
    BlockStmt* finallyBody = nullptr;
    TryStmt() : Stmt(NodeKind::TryStmt) {}
};
struct MatchArm {
    std::string text; // textual pattern for v0
    Expr* guard = nullptr;
    Stmt* body = nullptr;
    bool isDefault = false;
};
struct MatchStmt : Stmt {
    Expr* subject = nullptr;
    std::vector<MatchArm> arms;
    MatchStmt() : Stmt(NodeKind::MatchStmt) {}
};
struct LaunchStmt : Stmt {
    Stmt* body = nullptr;
    LaunchStmt() : Stmt(NodeKind::LaunchStmt) {}
};

// ---------------------------------------------------------------------------
// Declarations
// ---------------------------------------------------------------------------
struct NamespaceDecl : Decl {
    std::string name;
    NamespaceDecl() : Decl(NodeKind::NamespaceDecl) {}
};
struct ImportDecl : Decl {
    std::string path;
    std::vector<std::string> items;
    std::string alias;
    bool wildcard = false;
    ImportDecl() : Decl(NodeKind::ImportDecl) {}
};
struct FnDecl : Decl {
    std::string name;
    std::vector<TypeParam> typeParams;
    std::vector<Param*> params;
    TypeRef* returnType = nullptr;
    BlockStmt* body = nullptr; // null for abstract/interface
    FnDecl() : Decl(NodeKind::FnDecl) {}
};
struct FieldDecl : Decl {
    bool isVar = false;
    std::string name;
    TypeRef* type = nullptr;
    Expr* init = nullptr;
    FieldDecl() : Decl(NodeKind::FieldDecl) {}
};
struct ConstructorDecl : Decl {
    std::vector<Param*> params;
    BlockStmt* body = nullptr;
    ConstructorDecl() : Decl(NodeKind::ConstructorDecl) {}
};
struct DestructorDecl : Decl {
    BlockStmt* body = nullptr;
    DestructorDecl() : Decl(NodeKind::DestructorDecl) {}
};
struct ClassDecl : Decl {
    enum class Shape { Class, Struct, Interface };
    Shape shape = Shape::Class;
    std::string name;
    std::vector<TypeParam> typeParams;
    TypeRef* baseClass = nullptr;
    std::vector<TypeRef*> interfaces;
    std::vector<Decl*> members;
    ClassDecl() : Decl(NodeKind::ClassDecl) {}
};
struct EnumCase {
    std::string name;
    std::vector<Param*> payload;
    Expr* discriminant = nullptr;
};
struct EnumDecl : Decl {
    std::string name;
    std::vector<TypeParam> typeParams;
    std::vector<TypeRef*> interfaces;
    std::vector<EnumCase> cases;
    std::vector<Decl*> members;
    EnumDecl() : Decl(NodeKind::EnumDecl) {}
};
struct ExtensionDecl : Decl {
    TypeRef* target = nullptr;
    std::vector<TypeParam> typeParams;
    std::vector<Decl*> members;
    ExtensionDecl() : Decl(NodeKind::ExtensionDecl) {}
};

// ---------------------------------------------------------------------------
// Root
// ---------------------------------------------------------------------------
struct Module {
    std::string fileName;
    NamespaceDecl* namespaceDecl = nullptr;
    std::vector<ImportDecl*> imports;
    std::vector<Decl*> declarations;
};

void printModule(const Module& module, std::ostream& os);

} // namespace mlang::ast

#endif
