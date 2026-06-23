// mbuild - the MLang build tool. It orchestrates mango (dependencies) and
// mlangc (compilation) over the build graph (docs/design/22-mbuild.md).
//
// v0 scaffold: `new` scaffolds a project, `check` runs the real frontend over a
// source file, and `build`/`run` report that they need the native backend. The
// incremental, parallel, cached build driver is layered on next.
#include "mlang/parser/Parser.hpp"
#include "mlang/support/Arena.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

int cmdNew(const std::string& name) {
    const fs::path root = name;
    if (fs::exists(root)) {
        std::cerr << "mbuild: '" << name << "' already exists\n";
        return 1;
    }
    fs::create_directories(root / "src");
    fs::create_directories(root / "tests");

    std::ofstream manifest(root / "mango.json");
    manifest << "{\n"
                "  \"name\": \"" << name << "\",\n"
                "  \"version\": \"0.1.0\",\n"
                "  \"edition\": \"2026\",\n"
                "  \"entry\": \"src/main.m\",\n"
                "  \"dependencies\": {}\n"
                "}\n";

    std::ofstream main(root / "src" / "main.m");
    main << "namespace " << name << "\n\n"
            "import std.io.Console\n\n"
            "public fn main() {\n"
            "    Console.println(\"Hello from " << name << "!\")\n"
            "}\n";

    std::cout << "Created MLang project '" << name << "'\n"
              << "  " << name << "/mango.json\n"
              << "  " << name << "/src/main.m\n";
    return 0;
}

int cmdCheck(const std::string& path) {
    const std::string target = path.empty() ? "src/main.m" : path;
    std::ifstream in(target, std::ios::binary);
    if (!in) {
        std::cerr << "mbuild: cannot open '" << target << "'\n";
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    mlang::support::SourceManager sources;
    const auto file = sources.addBuffer(target, ss.str());
    mlang::support::DiagnosticEngine diags(sources);
    mlang::support::Arena arena;
    mlang::parser::Parser parser(arena, sources, file, diags);
    (void)parser.parseModule();
    diags.printAll(std::cerr, true);
    if (!diags.hasErrors()) {
        std::cout << "check: " << target << " ok\n";
    }
    return diags.hasErrors() ? 1 : 0;
}

void usage() {
    std::cout << "mbuild - the MLang build tool\n"
                 "USAGE:\n"
                 "    mbuild new <name>      scaffold a new project\n"
                 "    mbuild check [file]    type-check the frontend (default src/main.m)\n"
                 "    mbuild build           build the project (needs the native backend)\n"
                 "    mbuild run             build and run (needs the native backend)\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string cmd = argv[1];
    if (cmd == "new" && argc >= 3) {
        return cmdNew(argv[2]);
    }
    if (cmd == "check") {
        return cmdCheck(argc >= 3 ? argv[2] : std::string());
    }
    if (cmd == "build" || cmd == "run") {
        std::cerr << "mbuild: '" << cmd
                  << "' requires the native backend (build mlangc with "
                     "-DMLANG_ENABLE_LLVM=ON). Use 'mbuild check' for frontend "
                     "validation today.\n";
        return 0;
    }
    if (cmd == "--help" || cmd == "help") {
        usage();
        return 0;
    }
    std::cerr << "mbuild: unknown command '" << cmd << "'\n";
    usage();
    return 2;
}
