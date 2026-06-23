// A tiny dependency-free test harness for the frontend. Each TEST registers a
// function; main runs them all and reports failures. Kept minimal on purpose -
// the full MLang test story lives in the language's own framework (Chapter 26).
#include "mlang/ast/Ast.hpp"
#include "mlang/lexer/Lexer.hpp"
#include "mlang/parser/Parser.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
std::string g_current;

struct Case {
    std::string name;
    std::function<void()> fn;
};

std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

struct Register {
    Register(std::string name, std::function<void()> fn) {
        registry().push_back({std::move(name), std::move(fn)});
    }
};

#define TEST(name)                                                                      \
    static void name();                                                                 \
    static const Register reg_##name(#name, name);                                       \
    static void name()

void check(bool cond, const std::string& msg) {
    if (!cond) {
        ++g_failures;
        std::cerr << "  FAIL [" << g_current << "]: " << msg << '\n';
    }
}

using namespace mlang;

std::vector<lexer::Token> lex(support::SourceManager& sm, const std::string& src) {
    const auto file = sm.addBuffer("<test>", src);
    support::DiagnosticEngine diags(sm);
    lexer::Lexer lexer(sm, file, diags);
    return lexer.tokenize();
}

ast::Module parse(support::SourceManager& sm, support::Arena& arena,
                  support::DiagnosticEngine& diags, const std::string& src) {
    const auto file = sm.addBuffer("<test>", src);
    parser::Parser parser(arena, sm, file, diags);
    return parser.parseModule();
}

// ---------------------------------------------------------------------------

TEST(lexer_keywords_and_operators) {
    support::SourceManager sm;
    auto toks = lex(sm, "fn main() { let x: 1 + 2 }");
    check(toks.front().kind == lexer::TokenKind::KwFn, "first token is 'fn'");
    check(toks.back().kind == lexer::TokenKind::EndOfFile, "last token is EOF");
    bool sawPlus = false;
    for (const auto& t : toks) {
        if (t.kind == lexer::TokenKind::Plus) sawPlus = true;
    }
    check(sawPlus, "found '+' operator");
}

TEST(lexer_numbers_and_strings) {
    support::SourceManager sm;
    auto toks = lex(sm, R"(let a: 3.14  let b: 0xFF  let c: "hi")");
    int floats = 0, ints = 0, strings = 0;
    for (const auto& t : toks) {
        if (t.kind == lexer::TokenKind::FloatLiteral) ++floats;
        if (t.kind == lexer::TokenKind::IntLiteral) ++ints;
        if (t.kind == lexer::TokenKind::StringLiteral) ++strings;
    }
    check(floats == 1, "one float literal");
    check(ints == 1, "one int literal (hex)");
    check(strings == 1, "one string literal");
}

TEST(lexer_shift_vs_compare) {
    support::SourceManager sm;
    auto toks = lex(sm, "a << b >= c");
    check(toks[1].kind == lexer::TokenKind::Shl, "'<<' lexes as Shl");
    check(toks[3].kind == lexer::TokenKind::Ge, "'>=' lexes as Ge");
}

TEST(parser_function_with_body) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    auto module = parse(sm, arena, diags,
                        "namespace t\nfn main() { let x: 1 + 2 * 3 }\n");
    check(!diags.hasErrors(), "function parses without errors");
    check(module.namespaceDecl != nullptr, "namespace parsed");
    check(module.declarations.size() == 1, "one declaration");
    if (!module.declarations.empty()) {
        check(module.declarations[0]->kind == ast::NodeKind::FnDecl, "decl is a function");
    }
}

TEST(parser_class_with_members) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    auto module = parse(sm, arena, diags,
                        "public class Account ext Entity impl Comparable {\n"
                        "  private var balance: Int\n"
                        "  public fn deposit(amount: Int) { balance = balance + amount }\n"
                        "}\n");
    check(!diags.hasErrors(), "class parses without errors");
    check(module.declarations.size() == 1, "one declaration");
    if (!module.declarations.empty() &&
        module.declarations[0]->kind == ast::NodeKind::ClassDecl) {
        auto* cls = static_cast<ast::ClassDecl*>(module.declarations[0]);
        check(cls->name == "Account", "class name is Account");
        check(cls->baseClass != nullptr, "has base class");
        check(cls->interfaces.size() == 1, "implements one interface");
        check(cls->members.size() == 2, "has two members");
    } else {
        check(false, "decl is a class");
    }
}

TEST(parser_precedence) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    // 1 + 2 * 3 should parse as 1 + (2 * 3): top binary op is '+'.
    auto module = parse(sm, arena, diags, "fn f() { let r: 1 + 2 * 3 }\n");
    check(!diags.hasErrors(), "expression parses");
}

TEST(parser_elvis_and_try_and_member_type) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    auto module = parse(sm, arena, diags,
                        "fn f(input: String?): Int {\n"
                        "  let value: input ?: \"default\"\n"
                        "  let parsed: parse(value)?\n"
                        "  return 0\n"
                        "}\n"
                        "class Box { private var items: List<Int> }\n");
    check(!diags.hasErrors(), "elvis, try-propagation, and member generic type parse");
    check(module.declarations.size() == 2, "function and class parsed");
}

TEST(parser_scope_and_launch_expressions) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    auto module = parse(sm, arena, diags,
                        "async fn handle(): String {\n"
                        "  return scope {\n"
                        "    let u: launch fetch(1)\n"
                        "    await u\n"
                        "  }\n"
                        "}\n");
    check(!diags.hasErrors(), "scope and launch expressions parse");
}

TEST(parser_recovers_from_error) {
    support::SourceManager sm;
    support::Arena arena;
    support::DiagnosticEngine diags(sm);
    // Garbage between two good functions; parser should still find both.
    auto module = parse(sm, arena, diags,
                        "fn a() {}\n @#$ \n fn b() {}\n");
    check(diags.hasErrors(), "error reported for garbage");
    check(module.declarations.size() >= 2, "both functions still recovered");
}

} // namespace

int main() {
    for (const auto& c : registry()) {
        g_current = c.name;
        c.fn();
    }
    if (g_failures == 0) {
        std::cout << "All " << registry().size() << " frontend tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) failed.\n";
    return 1;
}
