#ifndef MLANG_SUPPORT_DIAGNOSTIC_HPP
#define MLANG_SUPPORT_DIAGNOSTIC_HPP

#include "mlang/support/SourceManager.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace mlang::support {

enum class Severity { Note, Warning, Error };

struct Diagnostic {
    Severity severity = Severity::Error;
    std::string code;    // stable code, e.g. "E0001"
    std::string message;
    SourceRange range;
    std::vector<std::string> notes;
};

// Collects diagnostics during a compilation and renders them with source
// context. Sorting by location keeps output deterministic regardless of the
// order in which phases report.
class DiagnosticEngine {
public:
    explicit DiagnosticEngine(const SourceManager& sources) : sources_(sources) {}

    void report(Diagnostic diag);

    void error(SourceRange range, std::string code, std::string message);
    void warning(SourceRange range, std::string code, std::string message);

    [[nodiscard]] bool hasErrors() const noexcept { return errorCount_ > 0; }
    [[nodiscard]] std::size_t errorCount() const noexcept { return errorCount_; }
    [[nodiscard]] std::size_t count() const noexcept { return diagnostics_.size(); }

    void printAll(std::ostream& os, bool color = true) const;

private:
    const SourceManager& sources_;
    std::vector<Diagnostic> diagnostics_;
    std::size_t errorCount_ = 0;
};

} // namespace mlang::support

#endif
