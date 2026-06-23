#include "mlang/ast/Ast.hpp"
#include "mlang/lexer/Lexer.hpp"
#include "mlang/parser/Parser.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef MLANG_VERSION
#define MLANG_VERSION "0.1.0"
#endif

namespace {

enum class EmitKind { Tokens, Ast, Ir, Llvm, Obj, Exe };

struct Options {
    EmitKind emit = EmitKind::Ast;
    std::string input;
    bool color = true;
};

void printUsage(std::ostream& os) {
    os << "mlangc " << MLANG_VERSION << " - the MLang compiler\n\n"
       << "USAGE:\n"
       << "    mlangc [OPTIONS] <input.m>\n\n"
       << "OPTIONS:\n"
       << "    --emit=<stage>   tokens | ast | ir | llvm | obj | exe (default: ast)\n"
       << "    --no-color       disable colored diagnostics\n"
       << "    --version        print version and exit\n"
       << "    --help           print this help and exit\n";
}

bool parseArgs(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(std::cout);
            std::exit(0);
        } else if (arg == "--version") {
            std::cout << "mlangc " << MLANG_VERSION << '\n';
            std::exit(0);
        } else if (arg == "--no-color") {
            opts.color = false;
        } else if (arg.starts_with("--emit=")) {
            const std::string_view stage = arg.substr(7);
            if (stage == "tokens") opts.emit = EmitKind::Tokens;
            else if (stage == "ast") opts.emit = EmitKind::Ast;
            else if (stage == "ir") opts.emit = EmitKind::Ir;
            else if (stage == "llvm") opts.emit = EmitKind::Llvm;
            else if (stage == "obj") opts.emit = EmitKind::Obj;
            else if (stage == "exe") opts.emit = EmitKind::Exe;
            else {
                std::cerr << "mlangc: unknown emit stage '" << stage << "'\n";
                return false;
            }
        } else if (arg.starts_with("-")) {
            std::cerr << "mlangc: unknown option '" << arg << "'\n";
            return false;
        } else {
            opts.input = std::string(arg);
        }
    }
    if (opts.input.empty()) {
        std::cerr << "mlangc: no input file\n\n";
        printUsage(std::cerr);
        return false;
    }
    return true;
}

void emitTokens(const mlang::support::SourceManager& sm, mlang::support::FileId file,
                mlang::support::DiagnosticEngine& diags) {
    mlang::lexer::Lexer lexer(sm, file, diags);
    for (auto tokens = lexer.tokenize(); const auto& tok : tokens) {
        const auto lc = sm.lineColumn(tok.range.begin);
        std::cout << lc.line << ':' << lc.column << "\t"
                  << mlang::lexer::tokenKindName(tok.kind);
        if (tok.kind != mlang::lexer::TokenKind::EndOfFile) {
            std::cout << "\t'" << sm.spanText(tok.range) << '\'';
        }
        std::cout << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parseArgs(argc, argv, opts)) {
        return 2;
    }

    mlang::support::SourceManager sources;
    const auto fileId = sources.addFile(opts.input);
    if (!fileId) {
        std::cerr << "mlangc: cannot open '" << opts.input << "'\n";
        return 2;
    }

    mlang::support::DiagnosticEngine diags(sources);

    switch (opts.emit) {
    case EmitKind::Tokens:
        emitTokens(sources, *fileId, diags);
        break;
    case EmitKind::Ast: {
        mlang::support::Arena arena;
        mlang::parser::Parser parser(arena, sources, *fileId, diags);
        const mlang::ast::Module module = parser.parseModule();
        mlang::ast::printModule(module, std::cout);
        break;
    }
    case EmitKind::Ir:
    case EmitKind::Llvm:
    case EmitKind::Obj:
    case EmitKind::Exe: {
        // Front-end runs to validate the program; the backend stages past the
        // AST are under construction (see docs/design/37-development-phases.md).
        mlang::support::Arena arena;
        mlang::parser::Parser parser(arena, sources, *fileId, diags);
        (void)parser.parseModule();
        std::cerr << "mlangc: requested stage is not yet available in this build; "
                     "the frontend ran and reported any diagnostics below.\n";
        break;
    }
    }

    diags.printAll(std::cerr, opts.color);
    return diags.hasErrors() ? 1 : 0;
}
