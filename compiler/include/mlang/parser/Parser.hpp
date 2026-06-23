#ifndef MLANG_PARSER_PARSER_HPP
#define MLANG_PARSER_PARSER_HPP

#include "mlang/ast/Ast.hpp"
#include "mlang/lexer/Token.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <string>
#include <vector>

namespace mlang::parser {

// Recursive-descent parser with a Pratt expression sub-parser. Implements a
// pragmatic subset of the grammar in docs/grammar/mlang.ebnf. Recovers at
// declaration and statement boundaries instead of aborting on the first error.
class Parser {
public:
    Parser(support::Arena& arena, const support::SourceManager& sources,
           support::FileId file, support::DiagnosticEngine& diags);

    ast::Module parseModule();

private:
    const lexer::Token& peek(std::size_t ahead = 0) const;
    lexer::TokenKind kind(std::size_t ahead = 0) const;
    const lexer::Token& advance();
    bool check(lexer::TokenKind k) const;
    bool match(lexer::TokenKind k);
    bool expect(lexer::TokenKind k, const char* what);
    bool atEnd() const;
    std::string spelling(const lexer::Token& tok) const;

    // Declarations
    ast::Decl* parseDeclaration();
    void parseAnnotations(std::vector<ast::Annotation>& out);
    ast::Modifiers parseModifiers();
    ast::ImportDecl* parseImport();
    ast::FnDecl* parseFn();
    ast::FieldDecl* parseField(bool isConst);
    ast::ClassDecl* parseClassLike();
    ast::EnumDecl* parseEnum();
    ast::ExtensionDecl* parseExtension();
    ast::ConstructorDecl* parseConstructor();
    ast::DestructorDecl* parseDestructor();
    ast::Decl* parseMember();
    std::vector<ast::TypeParam> parseTypeParams();
    std::vector<ast::Param*> parseParamList();
    std::vector<ast::Argument> parseArgList();
    std::string parseQualifiedName();
    void skipWhereClause();
    void skipBalanced(lexer::TokenKind open, lexer::TokenKind close);

    // Types
    ast::TypeRef* parseType();
    ast::TypeRef* parsePrimaryType();
    bool consumeGenericClose();

    // Statements
    ast::Stmt* parseStatement();
    ast::BlockStmt* parseBlock();
    ast::Stmt* parseVarDecl(bool isConst);
    ast::Stmt* parseIf();
    ast::Stmt* parseWhile();
    ast::Stmt* parseFor();
    ast::Stmt* parseReturn();
    ast::Stmt* parseTry();
    ast::Stmt* parseMatch();
    ast::Stmt* parseDoWhile();

    // Expressions
    ast::Expr* parseExpression();
    ast::Expr* parseAssignment();
    ast::Expr* parseTernary();
    ast::Expr* parseBinary(int minPrec);
    ast::Expr* parseUnary();
    ast::Expr* parsePostfix();
    ast::Expr* parsePrimary();
    ast::Expr* tryParseLambda();
    bool looksLikeLambda() const;

    void consumeTerminator();
    void synchronize();
    // True if a top-level '=' appears before the statement ends. Used to tell
    // `let x: Int = v` (typed) from `let x: value` (colon introduces the value).
    bool scanHasTopLevelAssign() const;

    template <typename T>
    T* makeNode(support::SourceLoc start);

    support::Arena& arena_;
    const support::SourceManager& sources_;
    support::DiagnosticEngine& diags_;
    std::vector<lexer::Token> tokens_;
    std::size_t index_ = 0;
};

} // namespace mlang::parser

#endif
