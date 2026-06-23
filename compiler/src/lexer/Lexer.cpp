#include "mlang/lexer/Lexer.hpp"

#include <cctype>

namespace mlang::lexer {

namespace {
bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
           static_cast<unsigned char>(c) >= 0x80;
}
bool isIdentContinue(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
           static_cast<unsigned char>(c) >= 0x80;
}
bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
} // namespace

Lexer::Lexer(const support::SourceManager& sources, support::FileId file,
             support::DiagnosticEngine& diags)
    : sources_(sources), diags_(diags), file_(file), base_(sources.fileBase(file)),
      text_(sources.text(file)) {}

char Lexer::peek(std::size_t ahead) const {
    const std::size_t idx = pos_ + ahead;
    return idx < text_.size() ? text_[idx] : '\0';
}

bool Lexer::atEnd() const { return pos_ >= text_.size(); }

char Lexer::advance() { return text_[pos_++]; }

bool Lexer::match(char expected) {
    if (peek() == expected) {
        ++pos_;
        return true;
    }
    return false;
}

support::SourceRange Lexer::rangeFrom(std::uint32_t start) const {
    return support::SourceRange{base_ + start, pos_ - start};
}

Token Lexer::make(TokenKind kind, std::uint32_t start) {
    const bool startsLine = atLineStart_;
    atLineStart_ = false;
    return Token{kind, rangeFrom(start), startsLine};
}

void Lexer::skipTrivia() {
    while (!atEnd()) {
        const char c = peek();
        if (c == '\n') {
            atLineStart_ = true;
            ++pos_;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            ++pos_;
        } else if (c == '/' && peek(1) == '/') {
            while (!atEnd() && peek() != '\n') {
                ++pos_;
            }
        } else if (c == '/' && peek(1) == '*') {
            pos_ += 2;
            int depth = 1;
            while (!atEnd() && depth > 0) {
                if (peek() == '/' && peek(1) == '*') {
                    pos_ += 2;
                    ++depth;
                } else if (peek() == '*' && peek(1) == '/') {
                    pos_ += 2;
                    --depth;
                } else {
                    if (peek() == '\n') {
                        atLineStart_ = true;
                    }
                    ++pos_;
                }
            }
        } else {
            break;
        }
    }
}

Token Lexer::lexIdentifierOrKeyword(std::uint32_t start) {
    while (!atEnd() && isIdentContinue(peek())) {
        ++pos_;
    }
    const std::string_view spelling = text_.substr(start, pos_ - start);
    return make(keywordOrIdentifier(spelling), start);
}

Token Lexer::lexNumber(std::uint32_t start) {
    bool isFloat = false;
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'o' || peek(1) == 'b')) {
        pos_ += 2;
        while (!atEnd() && (isHexDigit(peek()) || peek() == '_')) {
            ++pos_;
        }
    } else {
        while (!atEnd() && (isDigit(peek()) || peek() == '_')) {
            ++pos_;
        }
        if (peek() == '.' && isDigit(peek(1))) {
            isFloat = true;
            ++pos_;
            while (!atEnd() && (isDigit(peek()) || peek() == '_')) {
                ++pos_;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            ++pos_;
            if (peek() == '+' || peek() == '-') {
                ++pos_;
            }
            while (!atEnd() && isDigit(peek())) {
                ++pos_;
            }
        }
    }
    // Type suffix (i32, u8, f64, L, ...): consume trailing identifier chars.
    while (!atEnd() && isIdentContinue(peek())) {
        if (peek() == 'f') {
            isFloat = true;
        }
        ++pos_;
    }
    return make(isFloat ? TokenKind::FloatLiteral : TokenKind::IntLiteral, start);
}

Token Lexer::lexString(std::uint32_t start, char quote) {
    const bool triple = quote == '"' && peek(1) == '"' && peek(2) == '"';
    if (triple) {
        pos_ += 3;
        while (!atEnd()) {
            if (peek() == '"' && peek(1) == '"' && peek(2) == '"') {
                pos_ += 3;
                return make(TokenKind::StringLiteral, start);
            }
            if (peek() == '\n') {
                atLineStart_ = true;
            }
            ++pos_;
        }
        diags_.error(rangeFrom(start), "E0002", "unterminated multiline string literal");
        return make(TokenKind::Error, start);
    }

    ++pos_; // opening quote
    while (!atEnd()) {
        const char c = peek();
        if (c == '\\') {
            pos_ += 2; // skip escape pair
            continue;
        }
        if (c == quote) {
            ++pos_;
            return make(quote == '\'' ? TokenKind::CharLiteral : TokenKind::StringLiteral, start);
        }
        if (c == '\n') {
            break;
        }
        ++pos_;
    }
    diags_.error(rangeFrom(start), "E0003", "unterminated string literal");
    return make(TokenKind::Error, start);
}

Token Lexer::lexRawString(std::uint32_t start) {
    ++pos_; // 'r'
    std::uint32_t hashes = 0;
    while (peek() == '#') {
        ++hashes;
        ++pos_;
    }
    if (peek() != '"') {
        diags_.error(rangeFrom(start), "E0004", "malformed raw string literal");
        return make(TokenKind::Error, start);
    }
    ++pos_; // opening quote
    while (!atEnd()) {
        if (peek() == '"') {
            std::uint32_t i = 1;
            while (i <= hashes && peek(i) == '#') {
                ++i;
            }
            if (i - 1 == hashes) {
                pos_ += 1 + hashes;
                return make(TokenKind::StringLiteral, start);
            }
        }
        if (peek() == '\n') {
            atLineStart_ = true;
        }
        ++pos_;
    }
    diags_.error(rangeFrom(start), "E0005", "unterminated raw string literal");
    return make(TokenKind::Error, start);
}

Token Lexer::next() {
    skipTrivia();
    const std::uint32_t start = pos_;
    if (atEnd()) {
        return make(TokenKind::EndOfFile, start);
    }

    const char c = peek();

    if (c == 'r' && (peek(1) == '"' || peek(1) == '#')) {
        return lexRawString(start);
    }
    if (isIdentStart(c)) {
        return lexIdentifierOrKeyword(start);
    }
    if (isDigit(c) || (c == '.' && isDigit(peek(1)))) {
        return lexNumber(start);
    }
    if (c == '"') {
        return lexString(start, '"');
    }
    if (c == '\'') {
        return lexString(start, '\'');
    }

    advance();
    switch (c) {
    case '(': return make(TokenKind::LParen, start);
    case ')': return make(TokenKind::RParen, start);
    case '{': return make(TokenKind::LBrace, start);
    case '}': return make(TokenKind::RBrace, start);
    case '[': return make(TokenKind::LBracket, start);
    case ']': return make(TokenKind::RBracket, start);
    case ',': return make(TokenKind::Comma, start);
    case ';': return make(TokenKind::Semicolon, start);
    case '@': return make(TokenKind::At, start);
    case '~': return make(TokenKind::Tilde, start);
    case ':': return make(match(':') ? TokenKind::ColonColon : TokenKind::Colon, start);
    case '.': return make(TokenKind::Dot, start);
    case '+': return make(match('=') ? TokenKind::PlusEq : TokenKind::Plus, start);
    case '-':
        if (match('>')) return make(TokenKind::Arrow, start);
        return make(match('=') ? TokenKind::MinusEq : TokenKind::Minus, start);
    case '*':
        if (match('*')) return make(TokenKind::StarStar, start);
        return make(match('=') ? TokenKind::StarEq : TokenKind::Star, start);
    case '/': return make(match('=') ? TokenKind::SlashEq : TokenKind::Slash, start);
    case '%': return make(match('=') ? TokenKind::PercentEq : TokenKind::Percent, start);
    case '^': return make(TokenKind::Caret, start);
    case '=':
        if (match('=')) return make(TokenKind::EqEq, start);
        return make(match('>') ? TokenKind::FatArrow : TokenKind::Assign, start);
    case '!': return make(match('=') ? TokenKind::NotEq : TokenKind::Bang, start);
    case '<':
        if (match('<')) return make(TokenKind::Shl, start);
        return make(match('=') ? TokenKind::Le : TokenKind::Lt, start);
    case '>':
        if (match('>')) return make(TokenKind::Shr, start);
        return make(match('=') ? TokenKind::Ge : TokenKind::Gt, start);
    case '&':
        if (match('&')) return make(match('=') ? TokenKind::AmpAmpEq : TokenKind::AmpAmp, start);
        return make(TokenKind::Amp, start);
    case '|':
        if (match('|')) return make(match('=') ? TokenKind::PipePipeEq : TokenKind::PipePipe, start);
        return make(TokenKind::Pipe, start);
    case '?':
        if (match('.')) return make(TokenKind::QuestionDot, start);
        if (match(':')) return make(TokenKind::QuestionColon, start); // Elvis
        if (match('?')) return make(match('=') ? TokenKind::QuestionQuestionEq
                                               : TokenKind::QuestionQuestion, start);
        return make(TokenKind::Question, start);
    default:
        break;
    }

    diags_.error(rangeFrom(start), "E0001", "unexpected character in source");
    return make(TokenKind::Error, start);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        Token tok = next();
        const bool eof = tok.kind == TokenKind::EndOfFile;
        tokens.push_back(tok);
        if (eof) {
            break;
        }
    }
    return tokens;
}

} // namespace mlang::lexer
