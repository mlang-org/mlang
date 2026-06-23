#ifndef MLANG_LEXER_LEXER_HPP
#define MLANG_LEXER_LEXER_HPP

#include "mlang/lexer/Token.hpp"
#include "mlang/support/Diagnostic.hpp"
#include "mlang/support/SourceManager.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace mlang::lexer {

// Hand-written, single-pass tokenizer. Emits Error tokens and keeps going on
// invalid input so the parser can report many problems at once.
class Lexer {
public:
    Lexer(const support::SourceManager& sources, support::FileId file,
          support::DiagnosticEngine& diags);

    Token next();
    std::vector<Token> tokenize();

private:
    char peek(std::size_t ahead = 0) const;
    bool atEnd() const;
    char advance();
    bool match(char expected);

    Token make(TokenKind kind, std::uint32_t start);
    support::SourceRange rangeFrom(std::uint32_t start) const;

    void skipTrivia();
    Token lexIdentifierOrKeyword(std::uint32_t start);
    Token lexNumber(std::uint32_t start);
    Token lexString(std::uint32_t start, char quote);
    Token lexRawString(std::uint32_t start);

    const support::SourceManager& sources_;
    support::DiagnosticEngine& diags_;
    support::FileId file_;
    support::SourceLoc base_;
    std::string_view text_;
    std::uint32_t pos_ = 0;
    bool atLineStart_ = true;
};

} // namespace mlang::lexer

#endif
