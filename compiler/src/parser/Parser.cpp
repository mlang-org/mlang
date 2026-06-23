#include "mlang/parser/Parser.hpp"

#include "mlang/lexer/Lexer.hpp"

namespace mlang::parser {

using lexer::TokenKind;

namespace {
int infixPrec(TokenKind k) {
    switch (k) {
    case TokenKind::QuestionQuestion: return 1;
    case TokenKind::PipePipe: return 2;
    case TokenKind::AmpAmp: return 3;
    case TokenKind::Pipe: return 4;
    case TokenKind::Caret: return 5;
    case TokenKind::Amp: return 6;
    case TokenKind::EqEq:
    case TokenKind::NotEq: return 7;
    case TokenKind::Lt:
    case TokenKind::Gt:
    case TokenKind::Le:
    case TokenKind::Ge:
    case TokenKind::KwIs:
    case TokenKind::KwAs: return 8;
    case TokenKind::Shl:
    case TokenKind::Shr: return 9;
    case TokenKind::Plus:
    case TokenKind::Minus: return 10;
    case TokenKind::Star:
    case TokenKind::Slash:
    case TokenKind::Percent: return 11;
    case TokenKind::StarStar: return 12;
    default: return 0;
    }
}

bool isAssignOp(TokenKind k) {
    switch (k) {
    case TokenKind::Assign:
    case TokenKind::PlusEq:
    case TokenKind::MinusEq:
    case TokenKind::StarEq:
    case TokenKind::SlashEq:
    case TokenKind::PercentEq:
    case TokenKind::AmpAmpEq:
    case TokenKind::PipePipeEq:
    case TokenKind::QuestionQuestionEq:
        return true;
    default:
        return false;
    }
}

bool isModifier(TokenKind k) {
    switch (k) {
    case TokenKind::KwPublic:
    case TokenKind::KwPrivate:
    case TokenKind::KwProtected:
    case TokenKind::KwInternal:
    case TokenKind::KwStatic:
    case TokenKind::KwAbstract:
    case TokenKind::KwVirtual:
    case TokenKind::KwOverride:
    case TokenKind::KwAsync:
    case TokenKind::KwReadonly:
    case TokenKind::KwSealed:
    case TokenKind::KwUnsafe:
        return true;
    default:
        return false;
    }
}
} // namespace

Parser::Parser(support::Arena& arena, const support::SourceManager& sources,
               support::FileId file, support::DiagnosticEngine& diags)
    : arena_(arena), sources_(sources), diags_(diags) {
    lexer::Lexer lex(sources, file, diags);
    tokens_ = lex.tokenize();
}

const lexer::Token& Parser::peek(std::size_t ahead) const {
    const std::size_t idx = index_ + ahead;
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
}

TokenKind Parser::kind(std::size_t ahead) const { return peek(ahead).kind; }

const lexer::Token& Parser::advance() {
    const lexer::Token& tok = tokens_[index_];
    if (index_ + 1 < tokens_.size()) {
        ++index_;
    }
    return tok;
}

bool Parser::check(TokenKind k) const { return kind() == k; }

bool Parser::match(TokenKind k) {
    if (check(k)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::atEnd() const { return kind() == TokenKind::EndOfFile; }

bool Parser::expect(TokenKind k, const char* what) {
    if (check(k)) {
        advance();
        return true;
    }
    diags_.error(peek().range, "E0100",
                 std::string("expected ") + what + ", found '" +
                     std::string(lexer::tokenKindName(kind())) + "'");
    return false;
}

std::string Parser::spelling(const lexer::Token& tok) const {
    return std::string(sources_.spanText(tok.range));
}

void Parser::consumeTerminator() { match(TokenKind::Semicolon); }

bool Parser::scanHasTopLevelAssign() const {
    int depth = 0;
    for (std::size_t i = index_; i < tokens_.size(); ++i) {
        const lexer::Token& t = tokens_[i];
        if (i > index_ && t.startsLine && depth == 0) {
            return false; // statement ended at a newline
        }
        switch (t.kind) {
        case TokenKind::LParen:
        case TokenKind::LBracket:
        case TokenKind::LBrace:
            ++depth;
            break;
        case TokenKind::RParen:
        case TokenKind::RBracket:
            --depth;
            break;
        case TokenKind::RBrace:
            if (depth == 0) {
                return false;
            }
            --depth;
            break;
        case TokenKind::Semicolon:
            if (depth == 0) {
                return false;
            }
            break;
        case TokenKind::Assign:
            if (depth == 0) {
                return true;
            }
            break;
        case TokenKind::EndOfFile:
            return false;
        default:
            break;
        }
    }
    return false;
}

void Parser::synchronize() {
    while (!atEnd()) {
        switch (kind()) {
        case TokenKind::KwFn:
        case TokenKind::KwClass:
        case TokenKind::KwStruct:
        case TokenKind::KwInterface:
        case TokenKind::KwEnum:
        case TokenKind::KwExtension:
        case TokenKind::KwLet:
        case TokenKind::KwVar:
        case TokenKind::KwConst:
        case TokenKind::RBrace:
            return;
        default:
            advance();
        }
    }
}

void Parser::skipBalanced(TokenKind open, TokenKind close) {
    if (!match(open)) {
        return;
    }
    int depth = 1;
    while (!atEnd() && depth > 0) {
        if (check(open)) {
            ++depth;
        } else if (check(close)) {
            --depth;
        }
        advance();
    }
}

std::string Parser::parseQualifiedName() {
    std::string name;
    if (check(TokenKind::Identifier)) {
        name = spelling(advance());
    } else {
        expect(TokenKind::Identifier, "identifier");
        return name;
    }
    while (kind() == TokenKind::Dot && kind(1) == TokenKind::Identifier) {
        advance();
        name += '.';
        name += spelling(advance());
    }
    return name;
}

template <typename T>
T* Parser::makeNode(support::SourceLoc start) {
    T* node = arena_.make<T>();
    node->range.begin = start;
    node->range.length = 0;
    return node;
}

// ---------------------------------------------------------------------------
// Module / declarations
// ---------------------------------------------------------------------------
ast::Module Parser::parseModule() {
    ast::Module module;
    const support::FileId file = sources_.fileForLoc(peek().range.begin);
    if (file != support::kInvalidFile) {
        module.fileName = std::string(sources_.name(file));
    }

    if (check(TokenKind::KwNamespace)) {
        const support::SourceLoc start = peek().range.begin;
        advance();
        auto* ns = makeNode<ast::NamespaceDecl>(start);
        ns->name = parseQualifiedName();
        consumeTerminator();
        module.namespaceDecl = ns;
    }

    while (check(TokenKind::KwImport)) {
        module.imports.push_back(parseImport());
    }

    while (!atEnd()) {
        const std::size_t before = index_;
        ast::Decl* decl = parseDeclaration();
        if (decl) {
            module.declarations.push_back(decl);
        }
        if (index_ == before) {
            advance(); // guarantee progress
        }
    }
    return module;
}

ast::ImportDecl* Parser::parseImport() {
    const support::SourceLoc start = peek().range.begin;
    advance(); // import
    auto* imp = makeNode<ast::ImportDecl>(start);
    imp->path = parseQualifiedName();
    if (match(TokenKind::Dot)) {
        if (match(TokenKind::Star)) {
            imp->wildcard = true;
        } else if (match(TokenKind::LBrace)) {
            while (!check(TokenKind::RBrace) && !atEnd()) {
                if (check(TokenKind::Identifier)) {
                    imp->items.push_back(spelling(advance()));
                    if (match(TokenKind::KwAs) && check(TokenKind::Identifier)) {
                        advance();
                    }
                }
                if (!match(TokenKind::Comma)) {
                    break;
                }
            }
            expect(TokenKind::RBrace, "'}'");
        }
    } else if (match(TokenKind::KwAs)) {
        if (check(TokenKind::Identifier)) {
            imp->alias = spelling(advance());
        }
    }
    consumeTerminator();
    return imp;
}

void Parser::parseAnnotations(std::vector<ast::Annotation>& out) {
    while (check(TokenKind::At)) {
        const support::SourceLoc start = peek().range.begin;
        advance();
        ast::Annotation ann;
        ann.range.begin = start;
        ann.name = parseQualifiedName();
        if (check(TokenKind::LParen)) {
            skipBalanced(TokenKind::LParen, TokenKind::RParen);
        }
        out.push_back(std::move(ann));
    }
}

ast::Modifiers Parser::parseModifiers() {
    ast::Modifiers m;
    while (isModifier(kind())) {
        switch (kind()) {
        case TokenKind::KwPublic: m.isPublic = true; break;
        case TokenKind::KwPrivate: m.isPrivate = true; break;
        case TokenKind::KwProtected: m.isProtected = true; break;
        case TokenKind::KwInternal: m.isInternal = true; break;
        case TokenKind::KwStatic: m.isStatic = true; break;
        case TokenKind::KwAbstract: m.isAbstract = true; break;
        case TokenKind::KwVirtual: m.isVirtual = true; break;
        case TokenKind::KwOverride: m.isOverride = true; break;
        case TokenKind::KwAsync: m.isAsync = true; break;
        case TokenKind::KwReadonly: m.isReadonly = true; break;
        case TokenKind::KwSealed: m.isSealed = true; break;
        case TokenKind::KwUnsafe: m.isUnsafe = true; break;
        default: break;
        }
        advance();
    }
    return m;
}

ast::Decl* Parser::parseDeclaration() {
    std::vector<ast::Annotation> annotations;
    parseAnnotations(annotations);
    const ast::Modifiers mods = parseModifiers();

    ast::Decl* decl = nullptr;
    switch (kind()) {
    case TokenKind::KwClass:
    case TokenKind::KwStruct:
    case TokenKind::KwInterface:
        decl = parseClassLike();
        break;
    case TokenKind::KwEnum:
        decl = parseEnum();
        break;
    case TokenKind::KwExtension:
        decl = parseExtension();
        break;
    case TokenKind::KwFn:
        decl = parseFn();
        break;
    case TokenKind::KwLet:
    case TokenKind::KwVar:
        decl = parseField(false);
        break;
    case TokenKind::KwConst:
        decl = parseField(true);
        break;
    case TokenKind::KwConstructor:
        decl = parseConstructor();
        break;
    case TokenKind::KwDestructor:
        decl = parseDestructor();
        break;
    default:
        diags_.error(peek().range, "E0101",
                     "expected a declaration, found '" +
                         std::string(lexer::tokenKindName(kind())) + "'");
        synchronize();
        return nullptr;
    }

    if (decl) {
        decl->annotations = std::move(annotations);
        decl->modifiers = mods;
    }
    return decl;
}

ast::Decl* Parser::parseMember() {
    std::vector<ast::Annotation> annotations;
    parseAnnotations(annotations);
    const ast::Modifiers mods = parseModifiers();

    ast::Decl* decl = nullptr;
    switch (kind()) {
    case TokenKind::KwFn: decl = parseFn(); break;
    case TokenKind::KwLet:
    case TokenKind::KwVar: decl = parseField(false); break;
    case TokenKind::KwConst: decl = parseField(true); break;
    case TokenKind::KwConstructor: decl = parseConstructor(); break;
    case TokenKind::KwDestructor: decl = parseDestructor(); break;
    default:
        diags_.error(peek().range, "E0102", "expected a class member");
        advance();
        return nullptr;
    }
    if (decl) {
        decl->annotations = std::move(annotations);
        decl->modifiers = mods;
    }
    return decl;
}

std::vector<ast::TypeParam> Parser::parseTypeParams() {
    std::vector<ast::TypeParam> params;
    advance(); // '<'
    while (!check(TokenKind::Gt) && !check(TokenKind::Shr) && !atEnd()) {
        ast::TypeParam tp;
        if (match(TokenKind::KwIn)) {
            tp.variance = -1;
        } else if (check(TokenKind::Identifier) && spelling(peek()) == "out") {
            advance();
            tp.variance = 1;
        }
        if (check(TokenKind::Identifier)) {
            tp.name = spelling(advance());
        }
        if (match(TokenKind::Colon)) {
            tp.bounds.push_back(parseType());
            while (match(TokenKind::Amp)) {
                tp.bounds.push_back(parseType());
            }
        }
        params.push_back(std::move(tp));
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    consumeGenericClose();
    return params;
}

std::vector<ast::Param*> Parser::parseParamList() {
    std::vector<ast::Param*> params;
    while (!check(TokenKind::RParen) && !atEnd()) {
        auto* p = arena_.make<ast::Param>();
        p->range.begin = peek().range.begin;
        if (match(TokenKind::KwReadonly)) {
            p->readonly = true;
        }
        if (check(TokenKind::Identifier)) {
            p->name = spelling(advance());
        }
        if (match(TokenKind::Colon)) {
            p->type = parseType();
        }
        if (match(TokenKind::Assign)) {
            p->defaultValue = parseExpression();
        }
        params.push_back(p);
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    return params;
}

void Parser::skipWhereClause() {
    if (!check(TokenKind::KwWhere)) {
        return;
    }
    while (!atEnd() && !check(TokenKind::LBrace)) {
        advance();
    }
}

ast::FnDecl* Parser::parseFn() {
    const support::SourceLoc start = peek().range.begin;
    advance(); // fn
    auto* fn = makeNode<ast::FnDecl>(start);
    if (check(TokenKind::Identifier)) {
        fn->name = spelling(advance());
    }
    if (check(TokenKind::Lt)) {
        fn->typeParams = parseTypeParams();
    }
    expect(TokenKind::LParen, "'('");
    fn->params = parseParamList();
    expect(TokenKind::RParen, "')'");
    if (match(TokenKind::Colon)) {
        fn->returnType = parseType();
    }
    skipWhereClause();
    if (check(TokenKind::LBrace)) {
        fn->body = parseBlock();
    } else {
        consumeTerminator();
    }
    return fn;
}

ast::FieldDecl* Parser::parseField(bool isConst) {
    const support::SourceLoc start = peek().range.begin;
    auto* field = makeNode<ast::FieldDecl>(start);
    field->isVar = check(TokenKind::KwVar);
    advance(); // let/var/const
    (void)isConst;
    if (check(TokenKind::Identifier)) {
        field->name = spelling(advance());
    }
    if (match(TokenKind::Colon)) {
        if (scanHasTopLevelAssign()) {
            field->type = parseType();
            expect(TokenKind::Assign, "'='");
            field->init = parseExpression();
        } else {
            field->init = parseExpression();
        }
    } else if (match(TokenKind::Assign)) {
        field->init = parseExpression();
    }
    consumeTerminator();
    return field;
}

ast::ConstructorDecl* Parser::parseConstructor() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* ctor = makeNode<ast::ConstructorDecl>(start);
    expect(TokenKind::LParen, "'('");
    ctor->params = parseParamList();
    expect(TokenKind::RParen, "')'");
    if (check(TokenKind::LBrace)) {
        ctor->body = parseBlock();
    }
    return ctor;
}

ast::DestructorDecl* Parser::parseDestructor() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* dtor = makeNode<ast::DestructorDecl>(start);
    expect(TokenKind::LParen, "'('");
    expect(TokenKind::RParen, "')'");
    if (check(TokenKind::LBrace)) {
        dtor->body = parseBlock();
    }
    return dtor;
}

ast::ClassDecl* Parser::parseClassLike() {
    const support::SourceLoc start = peek().range.begin;
    auto* cls = makeNode<ast::ClassDecl>(start);
    cls->shape = check(TokenKind::KwStruct)      ? ast::ClassDecl::Shape::Struct
                 : check(TokenKind::KwInterface) ? ast::ClassDecl::Shape::Interface
                                                 : ast::ClassDecl::Shape::Class;
    advance(); // class/struct/interface
    if (check(TokenKind::Identifier)) {
        cls->name = spelling(advance());
    }
    if (check(TokenKind::Lt)) {
        cls->typeParams = parseTypeParams();
    }
    if (match(TokenKind::KwExt)) {
        cls->baseClass = parseType();
    }
    if (match(TokenKind::KwImpl)) {
        cls->interfaces.push_back(parseType());
        while (match(TokenKind::Comma)) {
            cls->interfaces.push_back(parseType());
        }
    }
    skipWhereClause();
    expect(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !atEnd()) {
        const std::size_t before = index_;
        ast::Decl* member = parseMember();
        if (member) {
            cls->members.push_back(member);
        }
        if (index_ == before) {
            advance();
        }
    }
    expect(TokenKind::RBrace, "'}'");
    return cls;
}

ast::EnumDecl* Parser::parseEnum() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* en = makeNode<ast::EnumDecl>(start);
    if (check(TokenKind::Identifier)) {
        en->name = spelling(advance());
    }
    if (check(TokenKind::Lt)) {
        en->typeParams = parseTypeParams();
    }
    if (match(TokenKind::KwImpl)) {
        en->interfaces.push_back(parseType());
        while (match(TokenKind::Comma)) {
            en->interfaces.push_back(parseType());
        }
    }
    expect(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !check(TokenKind::Semicolon) && !atEnd()) {
        if (!check(TokenKind::Identifier)) {
            break;
        }
        ast::EnumCase ec;
        ec.name = spelling(advance());
        if (check(TokenKind::LParen)) {
            advance();
            ec.payload = parseParamList();
            expect(TokenKind::RParen, "')'");
        }
        if (match(TokenKind::Assign)) {
            ec.discriminant = parseExpression();
        }
        en->cases.push_back(std::move(ec));
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    if (match(TokenKind::Semicolon)) {
        while (!check(TokenKind::RBrace) && !atEnd()) {
            const std::size_t before = index_;
            ast::Decl* member = parseMember();
            if (member) {
                en->members.push_back(member);
            }
            if (index_ == before) {
                advance();
            }
        }
    }
    expect(TokenKind::RBrace, "'}'");
    return en;
}

ast::ExtensionDecl* Parser::parseExtension() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* ext = makeNode<ast::ExtensionDecl>(start);
    if (check(TokenKind::Lt)) {
        ext->typeParams = parseTypeParams();
    }
    ext->target = parseType();
    if (match(TokenKind::KwImpl)) {
        parseType();
        while (match(TokenKind::Comma)) {
            parseType();
        }
    }
    skipWhereClause();
    expect(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !atEnd()) {
        const std::size_t before = index_;
        ast::Decl* member = parseMember();
        if (member) {
            ext->members.push_back(member);
        }
        if (index_ == before) {
            advance();
        }
    }
    expect(TokenKind::RBrace, "'}'");
    return ext;
}

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
bool Parser::consumeGenericClose() {
    if (check(TokenKind::Gt)) {
        advance();
        return true;
    }
    if (check(TokenKind::Shr)) {
        tokens_[index_].kind = TokenKind::Gt; // split '>>' into '>' '>'
        return true;
    }
    diags_.error(peek().range, "E0103", "expected '>'");
    return false;
}

ast::TypeRef* Parser::parseType() {
    ast::TypeRef* t = parsePrimaryType();
    if (t && match(TokenKind::Question)) {
        t->nullable = true;
    }
    return t;
}

ast::TypeRef* Parser::parsePrimaryType() {
    auto* t = arena_.make<ast::TypeRef>();
    t->range.begin = peek().range.begin;

    if (match(TokenKind::KwAsync)) {
        ast::TypeRef* inner = parsePrimaryType();
        if (inner) {
            inner->asyncFn = true;
        }
        return inner;
    }
    if (match(TokenKind::LBracket)) {
        t->elem = parseType();
        if (match(TokenKind::Colon)) {
            t->form = ast::TypeRef::Form::Map;
            t->value = parseType();
        } else {
            t->form = ast::TypeRef::Form::Array;
        }
        expect(TokenKind::RBracket, "']'");
        return t;
    }
    if (match(TokenKind::LParen)) {
        std::vector<ast::TypeRef*> elems;
        while (!check(TokenKind::RParen) && !atEnd()) {
            elems.push_back(parseType());
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RParen, "')'");
        if (match(TokenKind::Arrow)) {
            t->form = ast::TypeRef::Form::Func;
            t->params = std::move(elems);
            t->ret = parseType();
            return t;
        }
        if (elems.size() == 1) {
            return elems[0];
        }
        t->form = ast::TypeRef::Form::Named;
        t->name = "Tuple";
        t->args = std::move(elems);
        return t;
    }

    t->form = ast::TypeRef::Form::Named;
    t->name = parseQualifiedName();
    if (check(TokenKind::Lt)) {
        advance();
        while (!check(TokenKind::Gt) && !check(TokenKind::Shr) && !atEnd()) {
            if (match(TokenKind::KwIn) || (check(TokenKind::Identifier) && spelling(peek()) == "out")) {
                // variance marker on a use-site type argument
                if (kind() == TokenKind::Identifier) {
                    advance();
                }
            }
            t->args.push_back(parseType());
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        consumeGenericClose();
    }
    return t;
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------
ast::BlockStmt* Parser::parseBlock() {
    const support::SourceLoc start = peek().range.begin;
    auto* block = makeNode<ast::BlockStmt>(start);
    expect(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !atEnd()) {
        const std::size_t before = index_;
        ast::Stmt* stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(stmt);
        }
        if (index_ == before) {
            advance();
        }
    }
    expect(TokenKind::RBrace, "'}'");
    return block;
}

ast::Stmt* Parser::parseStatement() {
    switch (kind()) {
    case TokenKind::LBrace: return parseBlock();
    case TokenKind::KwLet: return parseVarDecl(false);
    case TokenKind::KwVar: return parseVarDecl(false);
    case TokenKind::KwConst: return parseVarDecl(true);
    case TokenKind::KwIf: return parseIf();
    case TokenKind::KwWhile: return parseWhile();
    case TokenKind::KwFor: return parseFor();
    case TokenKind::KwDo: return parseDoWhile();
    case TokenKind::KwReturn: return parseReturn();
    case TokenKind::KwTry: return parseTry();
    case TokenKind::KwMatch: return parseMatch();
    case TokenKind::KwBreak: {
        auto* s = makeNode<ast::BreakStmt>(peek().range.begin);
        advance();
        consumeTerminator();
        return s;
    }
    case TokenKind::KwContinue: {
        auto* s = makeNode<ast::ContinueStmt>(peek().range.begin);
        advance();
        consumeTerminator();
        return s;
    }
    case TokenKind::KwThrow: {
        auto* s = makeNode<ast::ThrowStmt>(peek().range.begin);
        advance();
        s->value = parseExpression();
        consumeTerminator();
        return s;
    }
    case TokenKind::KwLaunch: {
        auto* s = makeNode<ast::LaunchStmt>(peek().range.begin);
        advance();
        if (check(TokenKind::LBrace)) {
            s->body = parseBlock();
        } else {
            auto* es = makeNode<ast::ExprStmt>(peek().range.begin);
            es->expr = parseExpression();
            consumeTerminator();
            s->body = es;
        }
        return s;
    }
    default: {
        auto* s = makeNode<ast::ExprStmt>(peek().range.begin);
        s->expr = parseExpression();
        consumeTerminator();
        return s;
    }
    }
}

ast::Stmt* Parser::parseVarDecl(bool isConst) {
    const support::SourceLoc start = peek().range.begin;
    auto* v = makeNode<ast::VarDeclStmt>(start);
    v->isVar = check(TokenKind::KwVar);
    v->isConst = isConst;
    advance();
    if (check(TokenKind::Identifier)) {
        v->name = spelling(advance());
    }
    if (match(TokenKind::Colon)) {
        if (scanHasTopLevelAssign()) {
            v->type = parseType();
            expect(TokenKind::Assign, "'='");
            v->init = parseExpression();
        } else {
            v->init = parseExpression();
        }
    } else if (match(TokenKind::Assign)) {
        v->init = parseExpression();
    }
    consumeTerminator();
    return v;
}

ast::Stmt* Parser::parseIf() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::IfStmt>(start);
    expect(TokenKind::LParen, "'('");
    s->cond = parseExpression();
    expect(TokenKind::RParen, "')'");
    s->thenBranch = parseStatement();
    if (match(TokenKind::KwElse)) {
        s->elseBranch = parseStatement();
    }
    return s;
}

ast::Stmt* Parser::parseWhile() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::WhileStmt>(start);
    expect(TokenKind::LParen, "'('");
    s->cond = parseExpression();
    expect(TokenKind::RParen, "')'");
    s->body = parseStatement();
    return s;
}

ast::Stmt* Parser::parseDoWhile() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::DoWhileStmt>(start);
    s->body = parseStatement();
    expect(TokenKind::KwWhile, "'while'");
    expect(TokenKind::LParen, "'('");
    s->cond = parseExpression();
    expect(TokenKind::RParen, "')'");
    consumeTerminator();
    return s;
}

ast::Stmt* Parser::parseFor() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::ForStmt>(start);
    expect(TokenKind::LParen, "'('");
    if (check(TokenKind::Identifier)) {
        s->varName = spelling(advance());
    }
    expect(TokenKind::KwIn, "'in'");
    s->iterable = parseExpression();
    expect(TokenKind::RParen, "')'");
    s->body = parseStatement();
    return s;
}

ast::Stmt* Parser::parseReturn() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::ReturnStmt>(start);
    if (!check(TokenKind::RBrace) && !check(TokenKind::Semicolon) && !atEnd() &&
        !peek().startsLine) {
        s->value = parseExpression();
    }
    consumeTerminator();
    return s;
}

ast::Stmt* Parser::parseTry() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::TryStmt>(start);
    s->body = parseBlock();
    while (check(TokenKind::KwCatch)) {
        advance();
        ast::CatchClause c;
        expect(TokenKind::LParen, "'('");
        if (check(TokenKind::Identifier)) {
            c.name = spelling(advance());
        }
        if (match(TokenKind::Colon)) {
            c.type = parseType();
        }
        expect(TokenKind::RParen, "')'");
        c.body = parseBlock();
        s->catches.push_back(std::move(c));
    }
    if (match(TokenKind::KwFinally)) {
        s->finallyBody = parseBlock();
    }
    return s;
}

ast::Stmt* Parser::parseMatch() {
    const support::SourceLoc start = peek().range.begin;
    advance();
    auto* s = makeNode<ast::MatchStmt>(start);
    expect(TokenKind::LParen, "'('");
    s->subject = parseExpression();
    expect(TokenKind::RParen, "')'");
    expect(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !atEnd()) {
        ast::MatchArm arm;
        if (match(TokenKind::KwCase)) {
            std::string text;
            while (!check(TokenKind::FatArrow) && !check(TokenKind::RBrace) && !atEnd()) {
                if (!text.empty()) {
                    text += ' ';
                }
                text += spelling(advance());
            }
            arm.text = text;
        } else if (match(TokenKind::KwDefault)) {
            arm.isDefault = true;
        } else {
            break;
        }
        expect(TokenKind::FatArrow, "'=>'");
        if (check(TokenKind::LBrace)) {
            arm.body = parseBlock();
        } else {
            auto* es = makeNode<ast::ExprStmt>(peek().range.begin);
            es->expr = parseExpression();
            consumeTerminator();
            arm.body = es;
        }
        s->arms.push_back(std::move(arm));
    }
    expect(TokenKind::RBrace, "'}'");
    return s;
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------
ast::Expr* Parser::parseExpression() {
    if (looksLikeLambda()) {
        return tryParseLambda();
    }
    return parseAssignment();
}

bool Parser::looksLikeLambda() const {
    if (check(TokenKind::Identifier) && kind(1) == TokenKind::FatArrow) {
        return true;
    }
    if (!check(TokenKind::LParen)) {
        return false;
    }
    int depth = 0;
    for (std::size_t i = index_; i < tokens_.size(); ++i) {
        const TokenKind k = tokens_[i].kind;
        if (k == TokenKind::LParen) {
            ++depth;
        } else if (k == TokenKind::RParen) {
            --depth;
            if (depth == 0) {
                return i + 1 < tokens_.size() && tokens_[i + 1].kind == TokenKind::FatArrow;
            }
        } else if (k == TokenKind::EndOfFile) {
            return false;
        }
    }
    return false;
}

ast::Expr* Parser::tryParseLambda() {
    const support::SourceLoc start = peek().range.begin;
    auto* lambda = makeNode<ast::LambdaExpr>(start);
    if (check(TokenKind::Identifier)) {
        auto* p = arena_.make<ast::Param>();
        p->name = spelling(advance());
        lambda->params.push_back(p);
    } else {
        expect(TokenKind::LParen, "'('");
        while (!check(TokenKind::RParen) && !atEnd()) {
            auto* p = arena_.make<ast::Param>();
            if (check(TokenKind::Identifier)) {
                p->name = spelling(advance());
            }
            if (match(TokenKind::Colon)) {
                p->type = parseType();
            }
            lambda->params.push_back(p);
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RParen, "')'");
    }
    expect(TokenKind::FatArrow, "'=>'");
    if (check(TokenKind::LBrace)) {
        lambda->blockBody = parseBlock();
    } else {
        lambda->exprBody = parseExpression();
    }
    return lambda;
}

ast::Expr* Parser::parseAssignment() {
    ast::Expr* left = parseTernary();
    if (isAssignOp(kind())) {
        const support::SourceLoc start = left ? left->range.begin : peek().range.begin;
        auto* a = makeNode<ast::AssignExpr>(start);
        a->op = kind();
        advance();
        a->target = left;
        a->value = parseAssignment();
        return a;
    }
    return left;
}

ast::Expr* Parser::parseTernary() {
    ast::Expr* cond = parseBinary(1);
    if (match(TokenKind::Question)) {
        const support::SourceLoc start = cond ? cond->range.begin : peek().range.begin;
        auto* t = makeNode<ast::TernaryExpr>(start);
        t->cond = cond;
        t->thenExpr = parseExpression();
        expect(TokenKind::Colon, "':'");
        t->elseExpr = parseExpression();
        return t;
    }
    return cond;
}

ast::Expr* Parser::parseBinary(int minPrec) {
    ast::Expr* left = parseUnary();
    for (;;) {
        const TokenKind op = kind();
        const int prec = infixPrec(op);
        if (prec == 0 || prec < minPrec) {
            break;
        }
        const support::SourceLoc start = left ? left->range.begin : peek().range.begin;
        advance();
        if (op == TokenKind::KwAs) {
            auto* c = makeNode<ast::CastExpr>(start);
            c->value = left;
            c->type = parseType();
            left = c;
            continue;
        }
        if (op == TokenKind::KwIs) {
            auto* c = makeNode<ast::IsExpr>(start);
            c->value = left;
            c->type = parseType();
            left = c;
            continue;
        }
        const bool rightAssoc = op == TokenKind::StarStar || op == TokenKind::QuestionQuestion;
        ast::Expr* right = parseBinary(rightAssoc ? prec : prec + 1);
        auto* b = makeNode<ast::BinaryExpr>(start);
        b->op = op;
        b->lhs = left;
        b->rhs = right;
        left = b;
    }
    return left;
}

ast::Expr* Parser::parseUnary() {
    const TokenKind k = kind();
    if (k == TokenKind::Bang || k == TokenKind::Minus || k == TokenKind::Plus ||
        k == TokenKind::Tilde) {
        const support::SourceLoc start = peek().range.begin;
        advance();
        auto* u = makeNode<ast::UnaryExpr>(start);
        u->op = k;
        u->operand = parseUnary();
        return u;
    }
    if (k == TokenKind::KwAwait) {
        const support::SourceLoc start = peek().range.begin;
        advance();
        auto* a = makeNode<ast::AwaitExpr>(start);
        a->operand = parseUnary();
        return a;
    }
    if (k == TokenKind::KwNew) {
        const support::SourceLoc start = peek().range.begin;
        advance();
        auto* n = makeNode<ast::NewExpr>(start);
        n->type = parseType();
        if (check(TokenKind::LParen)) {
            n->args = parseArgList();
        }
        return n;
    }
    return parsePostfix();
}

std::vector<ast::Argument> Parser::parseArgList() {
    std::vector<ast::Argument> args;
    expect(TokenKind::LParen, "'('");
    while (!check(TokenKind::RParen) && !atEnd()) {
        ast::Argument arg;
        if (check(TokenKind::Identifier) && kind(1) == TokenKind::Colon) {
            arg.name = spelling(advance());
            advance(); // ':'
        }
        arg.value = parseExpression();
        args.push_back(std::move(arg));
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    expect(TokenKind::RParen, "')'");
    return args;
}

ast::Expr* Parser::parsePostfix() {
    ast::Expr* e = parsePrimary();
    for (;;) {
        const support::SourceLoc start = e ? e->range.begin : peek().range.begin;
        if (match(TokenKind::Dot)) {
            auto* m = makeNode<ast::MemberExpr>(start);
            m->object = e;
            if (check(TokenKind::Identifier)) {
                m->name = spelling(advance());
            }
            e = m;
        } else if (match(TokenKind::QuestionDot)) {
            auto* m = makeNode<ast::MemberExpr>(start);
            m->object = e;
            m->safe = true;
            if (check(TokenKind::Identifier)) {
                m->name = spelling(advance());
            }
            e = m;
        } else if (check(TokenKind::LParen)) {
            auto* c = makeNode<ast::CallExpr>(start);
            c->callee = e;
            c->args = parseArgList();
            e = c;
        } else if (match(TokenKind::LBracket)) {
            auto* idx = makeNode<ast::IndexExpr>(start);
            idx->object = e;
            idx->index = parseExpression();
            expect(TokenKind::RBracket, "']'");
            e = idx;
        } else if (check(TokenKind::Bang)) {
            advance();
            auto* p = makeNode<ast::PostfixExpr>(start);
            p->op = TokenKind::Bang;
            p->operand = e;
            e = p;
        } else {
            break;
        }
    }
    return e;
}

ast::Expr* Parser::parsePrimary() {
    const support::SourceLoc start = peek().range.begin;
    switch (kind()) {
    case TokenKind::IntLiteral:
    case TokenKind::FloatLiteral:
    case TokenKind::StringLiteral:
    case TokenKind::CharLiteral:
    case TokenKind::KwTrue:
    case TokenKind::KwFalse:
    case TokenKind::KwNull: {
        auto* lit = makeNode<ast::LiteralExpr>(start);
        lit->literalKind = kind();
        lit->text = spelling(advance());
        return lit;
    }
    case TokenKind::Identifier: {
        auto* id = makeNode<ast::IdentExpr>(start);
        id->name = spelling(advance());
        return id;
    }
    case TokenKind::KwThis: {
        advance();
        return makeNode<ast::ThisExpr>(start);
    }
    case TokenKind::KwSuper: {
        advance();
        return makeNode<ast::SuperExpr>(start);
    }
    case TokenKind::LParen: {
        advance();
        ast::Expr* e = parseExpression();
        expect(TokenKind::RParen, "')'");
        return e;
    }
    case TokenKind::LBracket: {
        advance();
        auto* arr = makeNode<ast::ArrayLiteralExpr>(start);
        while (!check(TokenKind::RBracket) && !atEnd()) {
            arr->elements.push_back(parseExpression());
            if (match(TokenKind::Colon)) {
                arr->elements.push_back(parseExpression()); // map value (v0: flat)
            }
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RBracket, "']'");
        return arr;
    }
    default: {
        diags_.error(peek().range, "E0104",
                     "expected an expression, found '" +
                         std::string(lexer::tokenKindName(kind())) + "'");
        advance(); // ensure progress
        return makeNode<ast::ErrorExpr>(start);
    }
    }
}

} // namespace mlang::parser
