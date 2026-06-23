// mls - the MLang language server. It is built on the compiler frontend so the
// editor and the compiler agree on the language (docs/design/24-mls.md).
//
// v0 scaffold: `mls --check <file>` runs the real frontend and prints
// diagnostics (the same engine the compiler uses); the full LSP stdio server
// (completion, hover, rename, incremental analysis) is the next increment.
#include "mlang/parser/Parser.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
int checkFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "mls: cannot open '" << path << "'\n";
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    mlang::support::SourceManager sources;
    const auto file = sources.addBuffer(path, ss.str());
    mlang::support::DiagnosticEngine diags(sources);
    mlang::support::Arena arena;
    mlang::parser::Parser parser(arena, sources, file, diags);
    (void)parser.parseModule();
    diags.printAll(std::cerr, true);
    if (!diags.hasErrors()) {
        std::cerr << "mls: " << path << " has no syntax diagnostics\n";
    }
    return diags.hasErrors() ? 1 : 0;
}
} // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--check") {
        return checkFile(argv[2]);
    }
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        std::cout << "mls - MLang language server\n"
                     "USAGE:\n"
                     "    mls --check <file>   run frontend diagnostics on a file\n"
                     "    mls --stdio          run the LSP server over stdio (scaffold)\n";
        return 0;
    }
    std::cerr << "mls: the LSP stdio server is under construction; "
                 "use 'mls --check <file>' for frontend diagnostics today.\n";
    return 0;
}
