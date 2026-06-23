#ifndef MLANG_LEXER_TOKEN_HPP
#define MLANG_LEXER_TOKEN_HPP

#include "mlang/support/SourceManager.hpp"

#include <cstdint>
#include <string_view>

namespace mlang::lexer {

enum class TokenKind : std::uint8_t {
    // Special
    EndOfFile,
    Error,

    // Literals and names
    Identifier,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,

    // Keywords
    KwNamespace, KwImport, KwAs,
    KwLet, KwVar, KwReadonly, KwConst,
    KwFn, KwReturn, KwYield,
    KwClass, KwStruct, KwInterface, KwEnum, KwExtension,
    KwAbstract, KwVirtual, KwOverride, KwStatic, KwSealed,
    KwConstructor, KwDestructor,
    KwExt, KwImpl, KwWhere,
    KwPublic, KwPrivate, KwProtected, KwInternal,
    KwIf, KwElse, KwMatch, KwCase, KwDefault,
    KwFor, KwWhile, KwDo, KwIn, KwBreak, KwContinue,
    KwTry, KwCatch, KwFinally, KwThrow,
    KwAsync, KwAwait, KwLaunch, KwScope,
    KwNew, KwThis, KwSuper, KwIs, KwUnsafe,
    KwTrue, KwFalse, KwNull,

    // Punctuation and operators
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, ColonColon, Dot, QuestionDot, Arrow, FatArrow, At,
    Plus, Minus, Star, Slash, Percent, StarStar,
    Amp, Pipe, Caret, Tilde, Shl, Shr,
    AmpAmp, PipePipe, Bang,
    Question, QuestionQuestion,
    Assign, PlusEq, MinusEq, StarEq, SlashEq, PercentEq,
    AmpAmpEq, PipePipeEq, QuestionQuestionEq,
    EqEq, NotEq, Lt, Gt, Le, Ge,
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    support::SourceRange range;
    bool startsLine = false;
};

std::string_view tokenKindName(TokenKind kind);

// Returns the keyword TokenKind for an identifier spelling, or Identifier.
TokenKind keywordOrIdentifier(std::string_view text);

} // namespace mlang::lexer

#endif
