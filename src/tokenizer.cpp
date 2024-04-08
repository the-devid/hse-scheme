#include "tokenizer.h"

#include "error.h"

#include <cctype>
#include <istream>
#include <set>
#include <string>
#include <variant>

namespace {

bool IsSign(int c) {
    return c == '+' || c == '-';
}

bool IsParen(int c) {
    return c == '(' || c == ')';
}

bool IsDigit(int c) {
    return std::isdigit(c);
}

bool IsDot(int c) {
    return c == '.';
}

bool IsQuote(int c) {
    return c == '\'';
}

BracketToken GetParen(std::istream* is) {
    if (is->get() == '(') {
        return BracketToken::OPEN;
    } else {
        return BracketToken::CLOSE;
    }
}

DotToken GetDot(std::istream* is) {
    is->ignore();
    return DotToken{};
}

QuoteToken GetQuote(std::istream* is) {
    is->ignore();
    return QuoteToken{};
}

Token GetConstantOrSign(std::istream* is) {
    bool is_negative = false;
    if (IsSign(is->peek())) {
        char sign = is->get();
        if (!IsDigit(is->peek())) {
            return SymbolToken{std::string(1, sign)};
        }
        is_negative = (sign == '-');
    }
    int value = 0;
    while (IsDigit(is->peek())) {
        value = value * 10 + (is->get() - '0');
    }
    if (is_negative) {
        value *= -1;
    }
    return ConstantToken{value};
}

bool IsFirstCharOfSymbol(int c) {
    return std::isalpha(c) || c == '<' || c == '=' || c == '>' || c == '*' || c == '/' || c == '#';
}
bool IsContinuingCharOfSymbol(int c) {
    return IsFirstCharOfSymbol(c) || std::isdigit(c) || c == '!' || c == '?' || c == '-';
}

SymbolToken GetSymbol(std::istream* is) {
    if (IsSign(is->peek())) {
        return SymbolToken{std::string(1, is->get())};
    }
    std::string result;
    if (!IsFirstCharOfSymbol(is->peek())) {
        throw SyntaxError("Tokenization failed at position " + std::to_string(is->tellg()) +
                          ": not a valid first character of a symbol token: \"" + std::string(1, is->peek()) + "\"");
    }
    result.push_back(is->get());
    while (is->peek() != EOF && !std::isspace(is->peek())) {
        if (!IsContinuingCharOfSymbol(is->peek())) {
            // throw SyntaxError("Tokenization failed at position " + std::to_string(is->tellg()) +
            //                   ": not a valid character inside of a symbol token: \"" +
            //                   std::string(1, is->peek()) + "\"");
            break;
        }
        result.push_back(is->get());
    }
    return SymbolToken{result};
}

} // namespace

Tokenizer::Tokenizer(std::istream* in) : is_(in), is_end_(false) {}

bool Tokenizer::IsEnd() {
    if (std::holds_alternative<std::monostate>(current_token_)) {
        IgnoreSpaces();
        return is_->peek() == EOF;
    }
    return is_end_;
}

void Tokenizer::IgnoreSpaces() {
    while (is_->peek() != EOF && std::isspace(is_->peek())) {
        is_->ignore();
    }
}

void Tokenizer::Next() {
    IgnoreSpaces();
    if (is_->peek() == EOF) {
        is_end_ = true;
        return;
    }
    if (IsParen(is_->peek())) {
        current_token_ = GetParen(is_);
    } else if (IsDot(is_->peek())) {
        current_token_ = GetDot(is_);
    } else if (IsQuote(is_->peek())) {
        current_token_ = GetQuote(is_);
    } else if (IsSign(is_->peek()) || IsDigit(is_->peek())) {
        current_token_ = GetConstantOrSign(is_);
    } else {
        current_token_ = GetSymbol(is_);
    }
}

Token Tokenizer::GetToken() {
    if (std::holds_alternative<std::monostate>(current_token_)) {
        // It is a first call, should read a token first.
        Next();
    }
    return current_token_;
}
