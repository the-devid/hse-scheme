#include "scheme.h"

#include "error.h"
#include "object.h"
#include "tokenizer.h"
#include "parser.h"

#include <sstream>

Interpreter::Interpreter() : global_context_(std::make_shared<Context>(Context::GetKeywords())) {
}

std::string Interpreter::Run(const std::string &s) {
    std::stringstream ss(s);
    Tokenizer tokenizer(&ss);
    auto ast = Read(&tokenizer);
    if (!tokenizer.IsEnd()) {
        throw SyntaxError("Garbage at the end of input");
    }
    auto result = ::Evaluate(ast, global_context_);
    return ::Serialize(result);
}
