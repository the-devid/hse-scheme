#pragma once

#include <variant>
#include <optional>
#include <istream>

struct SymbolToken {
    std::string name;

    bool operator==(const SymbolToken& other) const = default;
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const = default;
};

struct DotToken {
    bool operator==(const DotToken&) const = default;
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    int value;

    bool operator==(const ConstantToken& other) const = default;
};

using Token =
    std::variant<std::monostate, ConstantToken, BracketToken, SymbolToken, QuoteToken, DotToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in);

    bool IsEnd();

    void Next();

    Token GetToken();

private:
    std::istream* is_;
    Token current_token_;
    bool is_end_;

    void IgnoreSpaces();
};