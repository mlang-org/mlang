#include "mlang/ast/Ast.hpp"
#include "mlang/codegen/CodeGenBackend.hpp"
#include "mlang/ir/MirBuilder.hpp"
#include "mlang/lexer/Lexer.hpp"
#include "mlang/parser/Parser.hpp"
#include "mlang/sema/Sema.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"
#include "mlang/types/Type.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef MLANG_VERSION
#define MLANG_VERSION "0.1.0"
#endif

namespace {

enum class EmitKind { Tokens, Ast, Sema, Ir, Llvm, Obj, Exe };

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
       << "    --emit=<stage>   tokens | ast | sema | ir | llvm | obj | exe (default: ast)\n"
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
            else if (stage == "sema") opts.emit = EmitKind::Sema;
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
    case EmitKind::Sema: {
        mlang::support::Arena arena;
        mlang::parser::Parser parser(arena, sources, *fileId, diags);
        const mlang::ast::Module module = parser.parseModule();
        mlang::types::TypeContext types;
        mlang::sema::Sema sema(diags, types);
        sema.analyze(module);
        for (const auto& sym : sema.globals()) {
            const auto lc = sources.lineColumn(sym.declaredAt.begin);
            std::cout << "symbol " << sym.name << " (" << lc.line << ':' << lc.column << ")\n";
        }
        break;
    }
    case EmitKind::Ir: {
        mlang::support::Arena arena;
        mlang::parser::Parser parser(arena, sources, *fileId, diags);
        const mlang::ast::Module module = parser.parseModule();
        mlang::types::TypeContext types;
        mlang::sema::Sema sema(diags, types);
        sema.analyze(module);
        mlang::ir::MirBuilder builder(types);
        const mlang::ir::Module mir = builder.build(module);
        mir.print(std::cout);
        break;
    }
    case EmitKind::Llvm:
    case EmitKind::Obj:
    case EmitKind::Exe: {
        mlang::support::Arena arena;
        mlang::parser::Parser parser(arena, sources, *fileId, diags);
        const mlang::ast::Module module = parser.parseModule();
        mlang::types::TypeContext types;
        mlang::sema::Sema sema(diags, types);
        sema.analyze(module);
        mlang::ir::MirBuilder builder(types);
        const mlang::ir::Module mir = builder.build(module);
        if (auto backend = mlang::codegen::createBackend()) {
            mlang::codegen::TargetInfo target;
            backend->lowerModule(mir, target);
            std::cerr << "mlangc: native code generation is not wired up in this build.\n";
        } else {
            std::cerr << "mlangc: this build has no native backend "
                         "(configure with -DMLANG_ENABLE_LLVM=ON); "
                         "the frontend and MIR stages ran successfully.\n";
        }
        break;
    }
    }

    diags.printAll(std::cerr, opts.color);
    return diags.hasErrors() ? 1 : 0;
}
