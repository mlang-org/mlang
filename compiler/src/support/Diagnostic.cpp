#include "mlang/support/Diagnostic.hpp"

#include <algorithm>
#include <ostream>
#include <string_view>

namespace mlang::support {

namespace {
constexpr std::string_view kReset = "\033[0m";
constexpr std::string_view kBold = "\033[1m";
constexpr std::string_view kRed = "\033[31m";
constexpr std::string_view kYellow = "\033[33m";
constexpr std::string_view kCyan = "\033[36m";

std::string_view severityLabel(Severity s) {
    switch (s) {
    case Severity::Error:
        return "error";
    case Severity::Warning:
        return "warning";
    case Severity::Note:
        return "note";
    }
    return "diagnostic";
}

std::string_view severityColor(Severity s) {
    switch (s) {
    case Severity::Error:
        return kRed;
    case Severity::Warning:
        return kYellow;
    case Severity::Note:
        return kCyan;
    }
    return kReset;
}
} // namespace

void DiagnosticEngine::report(Diagnostic diag) {
    if (diag.severity == Severity::Error) {
        ++errorCount_;
    }
    diagnostics_.push_back(std::move(diag));
}

void DiagnosticEngine::error(SourceRange range, std::string code, std::string message) {
    report(Diagnostic{Severity::Error, std::move(code), std::move(message), range, {}});
}

void DiagnosticEngine::warning(SourceRange range, std::string code, std::string message) {
    report(Diagnostic{Severity::Warning, std::move(code), std::move(message), range, {}});
}

void DiagnosticEngine::printAll(std::ostream& os, bool color) const {
    std::vector<const Diagnostic*> ordered;
    ordered.reserve(diagnostics_.size());
    for (const auto& d : diagnostics_) {
        ordered.push_back(&d);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Diagnostic* a, const Diagnostic* b) {
                         return a->range.begin < b->range.begin;
                     });

    const auto col = [&](std::string_view code) { return color ? code : std::string_view{}; };

    for (const Diagnostic* d : ordered) {
        const LineColumn lc = sources_.lineColumn(d->range.begin);
        const FileId file = sources_.fileForLoc(d->range.begin);
        const std::string_view fileName = file != kInvalidFile ? sources_.name(file) : "<unknown>";

        os << col(kBold) << fileName << ':' << lc.line << ':' << lc.column << ": " << col(kReset);
        os << col(severityColor(d->severity)) << col(kBold) << severityLabel(d->severity);
        if (!d->code.empty()) {
            os << '[' << d->code << ']';
        }
        os << col(kReset) << ": " << d->message << '\n';

        const std::string_view line = sources_.lineText(d->range.begin);
        if (!line.empty()) {
            os << "    " << line << '\n';
            os << "    ";
            for (std::uint32_t i = 1; i < lc.column; ++i) {
                os << ' ';
            }
            os << col(severityColor(d->severity)) << '^';
            for (std::uint32_t i = 1; i < d->range.length; ++i) {
                os << '~';
            }
            os << col(kReset) << '\n';
        }

        for (const auto& note : d->notes) {
            os << "    " << col(kCyan) << "note" << col(kReset) << ": " << note << '\n';
        }
    }
}

} // namespace mlang::support
