#include "mlang/support/SourceManager.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

namespace mlang::support {

namespace {
std::vector<std::uint32_t> computeLineStarts(std::string_view text) {
    std::vector<std::uint32_t> starts;
    starts.push_back(0);
    for (std::uint32_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            starts.push_back(i + 1);
        }
    }
    return starts;
}
} // namespace

FileId SourceManager::addBuffer(std::string name, std::string contents) {
    Entry entry;
    entry.name = std::move(name);
    entry.contents = std::move(contents);
    entry.base = nextBase_;
    entry.lineStarts = computeLineStarts(entry.contents);

    // Reserve one extra slot so end-of-file locations remain unique per file.
    nextBase_ += static_cast<SourceLoc>(entry.contents.size()) + 1;
    files_.push_back(std::move(entry));
    return static_cast<FileId>(files_.size() - 1);
}

std::optional<FileId> SourceManager::addFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return addBuffer(path, ss.str());
}

std::string_view SourceManager::text(FileId file) const {
    return files_[file].contents;
}

std::string_view SourceManager::name(FileId file) const {
    return files_[file].name;
}

SourceLoc SourceManager::fileBase(FileId file) const {
    return files_[file].base;
}

FileId SourceManager::fileForLoc(SourceLoc loc) const {
    for (std::size_t i = files_.size(); i-- > 0;) {
        if (loc >= files_[i].base) {
            return static_cast<FileId>(i);
        }
    }
    return kInvalidFile;
}

std::string_view SourceManager::spanText(SourceRange range) const {
    if (!range.valid()) {
        return {};
    }
    const FileId file = fileForLoc(range.begin);
    if (file == kInvalidFile) {
        return {};
    }
    const Entry& entry = files_[file];
    const std::uint32_t offset = range.begin - entry.base;
    if (offset > entry.contents.size()) {
        return {};
    }
    const std::uint32_t len =
        std::min<std::uint32_t>(range.length,
                                static_cast<std::uint32_t>(entry.contents.size()) - offset);
    return std::string_view(entry.contents).substr(offset, len);
}

LineColumn SourceManager::lineColumn(SourceLoc loc) const {
    const FileId file = fileForLoc(loc);
    if (file == kInvalidFile) {
        return {};
    }
    const Entry& entry = files_[file];
    const std::uint32_t offset = loc - entry.base;
    const auto& starts = entry.lineStarts;
    auto it = std::upper_bound(starts.begin(), starts.end(), offset);
    const auto lineIdx = static_cast<std::uint32_t>(std::distance(starts.begin(), it) - 1);
    const std::uint32_t column = offset - starts[lineIdx] + 1;
    return LineColumn{lineIdx + 1, column};
}

std::string_view SourceManager::lineText(SourceLoc loc) const {
    const FileId file = fileForLoc(loc);
    if (file == kInvalidFile) {
        return {};
    }
    const Entry& entry = files_[file];
    const std::uint32_t offset = loc - entry.base;
    const auto& starts = entry.lineStarts;
    auto it = std::upper_bound(starts.begin(), starts.end(), offset);
    const auto lineIdx = static_cast<std::size_t>(std::distance(starts.begin(), it) - 1);
    const std::uint32_t start = starts[lineIdx];
    std::uint32_t stop = static_cast<std::uint32_t>(entry.contents.size());
    if (lineIdx + 1 < starts.size()) {
        stop = starts[lineIdx + 1];
        if (stop > start && entry.contents[stop - 1] == '\n') {
            --stop;
        }
        if (stop > start && entry.contents[stop - 1] == '\r') {
            --stop;
        }
    }
    return std::string_view(entry.contents).substr(start, stop - start);
}

} // namespace mlang::support
