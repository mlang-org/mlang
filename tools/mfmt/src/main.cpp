// mfmt - the MLang formatter. It shares the compiler frontend so it can never
// accept code the compiler rejects (docs/design/23-mfmt.md). This is the v0
// scaffold: it validates the file through the real parser and supports --check;
// the Wadler-style layout engine is the next increment, so the identity
// transform here is intentionally a safe, idempotent no-op.
#include "mlang/parser/Parser.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
bool parses(const std::string& path, std::string& contents) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "mfmt: cannot open '" << path << "'\n";
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    contents = ss.str();

    mlang::support::SourceManager sources;
    const auto file = sources.addBuffer(path, contents);
    mlang::support::DiagnosticEngine diags(sources);
    mlang::support::Arena arena;
    mlang::parser::Parser parser(arena, sources, file, diags);
    (void)parser.parseModule();
    diags.printAll(std::cerr, true);
    return !diags.hasErrors();
}
} // namespace

int main(int argc, char** argv) {
    bool checkOnly = false;
    bool writeInPlace = false;
    std::vector<std::string> inputs;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--check") checkOnly = true;
        else if (arg == "-w" || arg == "--write") writeInPlace = true;
        else if (arg == "--help") {
            std::cout << "mfmt - MLang formatter\n"
                         "USAGE: mfmt [--check | -w] <files...>\n";
            return 0;
        } else {
            inputs.push_back(arg);
        }
    }
    if (inputs.empty()) {
        std::cerr << "mfmt: no input files\n";
        return 2;
    }

    int status = 0;
    for (const auto& path : inputs) {
        std::string contents;
        if (!parses(path, contents)) {
            status = 1;
            continue;
        }
        // v0 canonical form == input (identity). Real layout pending.
        if (checkOnly) {
            continue; // already formatted by definition of the identity transform
        }
        if (writeInPlace) {
            continue; // nothing to change yet
        }
        std::cout << contents;
    }
    return status;
}
