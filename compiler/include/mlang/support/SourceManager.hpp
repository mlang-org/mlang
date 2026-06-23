#ifndef MLANG_SUPPORT_SOURCEMANAGER_HPP
#define MLANG_SUPPORT_SOURCEMANAGER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mlang::support {

using FileId = std::uint32_t;
using SourceLoc = std::uint32_t;

inline constexpr FileId kInvalidFile = 0xFFFFFFFFu;
inline constexpr SourceLoc kInvalidLoc = 0xFFFFFFFFu;

struct SourceRange {
    SourceLoc begin = kInvalidLoc;
    std::uint32_t length = 0;

    [[nodiscard]] SourceLoc end() const noexcept { return begin + length; }
    [[nodiscard]] bool valid() const noexcept { return begin != kInvalidLoc; }
};

struct LineColumn {
    std::uint32_t line = 0;   // 1-based
    std::uint32_t column = 0; // 1-based
};

// Owns every loaded source file and maps global SourceLocs back to a file plus
// a line/column. Files are concatenated into a virtual global offset space so a
// single 32-bit SourceLoc identifies any byte in the program.
class SourceManager {
public:
    FileId addBuffer(std::string name, std::string contents);
    std::optional<FileId> addFile(const std::string& path);

    [[nodiscard]] std::string_view text(FileId file) const;
    [[nodiscard]] std::string_view name(FileId file) const;
    [[nodiscard]] SourceLoc fileBase(FileId file) const;

    [[nodiscard]] std::string_view spanText(SourceRange range) const;
    [[nodiscard]] FileId fileForLoc(SourceLoc loc) const;
    [[nodiscard]] LineColumn lineColumn(SourceLoc loc) const;
    [[nodiscard]] std::string_view lineText(SourceLoc loc) const;

private:
    struct Entry {
        std::string name;
        std::string contents;
        SourceLoc base = 0;
        std::vector<std::uint32_t> lineStarts; // offsets within contents
    };

    std::vector<Entry> files_;
    SourceLoc nextBase_ = 0;
};

} // namespace mlang::support

#endif
