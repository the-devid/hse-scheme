#include "parser.h"
#include <memory>
#include <variant>

#include "error.h"
#include "object.h"
#include "tokenizer.h"

using std::make_shared;

//! We assume that opening bracket was read before we come here.
static std::shared_ptr<Object> ReadList(Tokenizer* tokenizer) {
    // tokenizer->Next();
    if (tokenizer->IsEnd()) {
        throw SyntaxError("List misses closing bracket");
    }
    if (tokenizer->GetToken() == Token{BracketToken::CLOSE}) {
        tokenizer->Next();
        return nullptr;
    }
    if (tokenizer->GetToken() == Token{DotToken{}}) {
        throw SyntaxError("Ill-formed dotted list");
    }
    auto result = make_shared<Cell>();
    result->SetFirst(Read(tokenizer));
    // tokenizer->Next();
    if (tokenizer->IsEnd()) {
        throw SyntaxError("List misses closing bracket");
    }
    if (tokenizer->GetToken() == Token{DotToken{}}) {
        tokenizer->Next();
        result->SetSecond(Read(tokenizer));
        if (tokenizer->IsEnd()) {
            throw SyntaxError("List misses closing bracket");
        }
        if (tokenizer->GetToken() != Token{BracketToken::CLOSE}) {
            throw SyntaxError("Ill-formed dotted list");
        }
        tokenizer->Next();
        return result;
    }
    result->SetSecond(ReadList(tokenizer));
    return result;
}

std::shared_ptr<Object> Read(Tokenizer* tokenizer) {
    if (tokenizer->IsEnd()) {
        throw SyntaxError{"Reached end while reading"};
    }
    const auto& token = tokenizer->GetToken();
    tokenizer->Next();
    if (token == Token{BracketToken::CLOSE}) {
        throw SyntaxError{"Invalid closing bracket"};
    }
    if (token == Token{BracketToken::OPEN}) {
        return ReadList(tokenizer);
    }
    if (std::holds_alternative<ConstantToken>(token)) {
        return make_shared<Number>(std::get<ConstantToken>(token).value);
    }
    if (std::holds_alternative<SymbolToken>(token)) {
        if (std::get<SymbolToken>(token).name == "#t") {
            return make_shared<Boolean>(true);
        }
        if (std::get<SymbolToken>(token).name == "#f") {
            return make_shared<Boolean>(false);
        }
        return make_shared<Symbol>(std::get<SymbolToken>(token).name);
    }
    if (std::holds_alternative<QuoteToken>(token)) {
        // tokenizer->Next();
        return make_shared<Cell>(make_shared<Symbol>("quote"),
                                 make_shared<Cell>(Read(tokenizer), nullptr));
    }
    throw SyntaxError{"Invalid token"};
}
