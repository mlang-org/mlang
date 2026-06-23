#include "mlang/lexer/Token.hpp"

#include <array>
#include <string_view>
#include <unordered_map>

namespace mlang::lexer {

std::string_view tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::EndOfFile: return "EOF";
    case TokenKind::Error: return "Error";
    case TokenKind::Identifier: return "Identifier";
    case TokenKind::IntLiteral: return "IntLiteral";
    case TokenKind::FloatLiteral: return "FloatLiteral";
    case TokenKind::StringLiteral: return "StringLiteral";
    case TokenKind::CharLiteral: return "CharLiteral";
    case TokenKind::KwNamespace: return "namespace";
    case TokenKind::KwImport: return "import";
    case TokenKind::KwAs: return "as";
    case TokenKind::KwLet: return "let";
    case TokenKind::KwVar: return "var";
    case TokenKind::KwReadonly: return "readonly";
    case TokenKind::KwConst: return "const";
    case TokenKind::KwFn: return "fn";
    case TokenKind::KwReturn: return "return";
    case TokenKind::KwYield: return "yield";
    case TokenKind::KwClass: return "class";
    case TokenKind::KwStruct: return "struct";
    case TokenKind::KwInterface: return "interface";
    case TokenKind::KwEnum: return "enum";
    case TokenKind::KwExtension: return "extension";
    case TokenKind::KwAbstract: return "abstract";
    case TokenKind::KwVirtual: return "virtual";
    case TokenKind::KwOverride: return "override";
    case TokenKind::KwStatic: return "static";
    case TokenKind::KwSealed: return "sealed";
    case TokenKind::KwConstructor: return "constructor";
    case TokenKind::KwDestructor: return "destructor";
    case TokenKind::KwExt: return "ext";
    case TokenKind::KwImpl: return "impl";
    case TokenKind::KwWhere: return "where";
    case TokenKind::KwPublic: return "public";
    case TokenKind::KwPrivate: return "private";
    case TokenKind::KwProtected: return "protected";
    case TokenKind::KwInternal: return "internal";
    case TokenKind::KwIf: return "if";
    case TokenKind::KwElse: return "else";
    case TokenKind::KwMatch: return "match";
    case TokenKind::KwCase: return "case";
    case TokenKind::KwDefault: return "default";
    case TokenKind::KwFor: return "for";
    case TokenKind::KwWhile: return "while";
    case TokenKind::KwDo: return "do";
    case TokenKind::KwIn: return "in";
    case TokenKind::KwBreak: return "break";
    case TokenKind::KwContinue: return "continue";
    case TokenKind::KwTry: return "try";
    case TokenKind::KwCatch: return "catch";
    case TokenKind::KwFinally: return "finally";
    case TokenKind::KwThrow: return "throw";
    case TokenKind::KwAsync: return "async";
    case TokenKind::KwAwait: return "await";
    case TokenKind::KwLaunch: return "launch";
    case TokenKind::KwScope: return "scope";
    case TokenKind::KwNew: return "new";
    case TokenKind::KwThis: return "this";
    case TokenKind::KwSuper: return "super";
    case TokenKind::KwIs: return "is";
    case TokenKind::KwUnsafe: return "unsafe";
    case TokenKind::KwTrue: return "true";
    case TokenKind::KwFalse: return "false";
    case TokenKind::KwNull: return "null";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::LBracket: return "[";
    case TokenKind::RBracket: return "]";
    case TokenKind::Comma: return ",";
    case TokenKind::Semicolon: return ";";
    case TokenKind::Colon: return ":";
    case TokenKind::ColonColon: return "::";
    case TokenKind::Dot: return ".";
    case TokenKind::QuestionDot: return "?.";
    case TokenKind::Arrow: return "->";
    case TokenKind::FatArrow: return "=>";
    case TokenKind::At: return "@";
    case TokenKind::Plus: return "+";
    case TokenKind::Minus: return "-";
    case TokenKind::Star: return "*";
    case TokenKind::Slash: return "/";
    case TokenKind::Percent: return "%";
    case TokenKind::StarStar: return "**";
    case TokenKind::Amp: return "&";
    case TokenKind::Pipe: return "|";
    case TokenKind::Caret: return "^";
    case TokenKind::Tilde: return "~";
    case TokenKind::Shl: return "<<";
    case TokenKind::Shr: return ">>";
    case TokenKind::AmpAmp: return "&&";
    case TokenKind::PipePipe: return "||";
    case TokenKind::Bang: return "!";
    case TokenKind::Question: return "?";
    case TokenKind::QuestionQuestion: return "??";
    case TokenKind::QuestionColon: return "?:";
    case TokenKind::Assign: return "=";
    case TokenKind::PlusEq: return "+=";
    case TokenKind::MinusEq: return "-=";
    case TokenKind::StarEq: return "*=";
    case TokenKind::SlashEq: return "/=";
    case TokenKind::PercentEq: return "%=";
    case TokenKind::AmpAmpEq: return "&&=";
    case TokenKind::PipePipeEq: return "||=";
    case TokenKind::QuestionQuestionEq: return "?\?=";
    case TokenKind::EqEq: return "==";
    case TokenKind::NotEq: return "!=";
    case TokenKind::Lt: return "<";
    case TokenKind::Gt: return ">";
    case TokenKind::Le: return "<=";
    case TokenKind::Ge: return ">=";
    }
    return "<unknown>";
}

TokenKind keywordOrIdentifier(std::string_view text) {
    static const std::unordered_map<std::string_view, TokenKind> keywords = {
        {"namespace", TokenKind::KwNamespace}, {"import", TokenKind::KwImport},
        {"as", TokenKind::KwAs}, {"let", TokenKind::KwLet}, {"var", TokenKind::KwVar},
        {"readonly", TokenKind::KwReadonly}, {"const", TokenKind::KwConst},
        {"fn", TokenKind::KwFn}, {"return", TokenKind::KwReturn}, {"yield", TokenKind::KwYield},
        {"class", TokenKind::KwClass}, {"struct", TokenKind::KwStruct},
        {"interface", TokenKind::KwInterface}, {"enum", TokenKind::KwEnum},
        {"extension", TokenKind::KwExtension}, {"abstract", TokenKind::KwAbstract},
        {"virtual", TokenKind::KwVirtual}, {"override", TokenKind::KwOverride},
        {"static", TokenKind::KwStatic}, {"sealed", TokenKind::KwSealed},
        {"constructor", TokenKind::KwConstructor}, {"destructor", TokenKind::KwDestructor},
        {"ext", TokenKind::KwExt}, {"impl", TokenKind::KwImpl}, {"where", TokenKind::KwWhere},
        {"public", TokenKind::KwPublic}, {"private", TokenKind::KwPrivate},
        {"protected", TokenKind::KwProtected}, {"internal", TokenKind::KwInternal},
        {"if", TokenKind::KwIf}, {"else", TokenKind::KwElse}, {"match", TokenKind::KwMatch},
        {"case", TokenKind::KwCase}, {"default", TokenKind::KwDefault},
        {"for", TokenKind::KwFor}, {"while", TokenKind::KwWhile}, {"do", TokenKind::KwDo},
        {"in", TokenKind::KwIn}, {"break", TokenKind::KwBreak}, {"continue", TokenKind::KwContinue},
        {"try", TokenKind::KwTry}, {"catch", TokenKind::KwCatch}, {"finally", TokenKind::KwFinally},
        {"throw", TokenKind::KwThrow}, {"async", TokenKind::KwAsync}, {"await", TokenKind::KwAwait},
        {"launch", TokenKind::KwLaunch}, {"scope", TokenKind::KwScope}, {"new", TokenKind::KwNew},
        {"this", TokenKind::KwThis}, {"super", TokenKind::KwSuper}, {"is", TokenKind::KwIs},
        {"unsafe", TokenKind::KwUnsafe}, {"true", TokenKind::KwTrue}, {"false", TokenKind::KwFalse},
        {"null", TokenKind::KwNull},
    };
    const auto it = keywords.find(text);
    return it == keywords.end() ? TokenKind::Identifier : it->second;
}

} // namespace mlang::lexer
