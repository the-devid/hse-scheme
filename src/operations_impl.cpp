#include "error.h"
#include "object.h"

#include <memory>
#include <vector>

#define DECLARE_FUNCTION(NAME)                                                                    \
    struct NAME : public Function {                                                               \
        virtual ObjectPtr Apply(ObjectPtr args, std::shared_ptr<Context> context) const override; \
    }

// Common function
DECLARE_FUNCTION(QuoteOp);

// Integer functions
DECLARE_FUNCTION(PlusOp);
DECLARE_FUNCTION(MinusOp);
DECLARE_FUNCTION(MultiplyOp);
DECLARE_FUNCTION(DivideOp);
DECLARE_FUNCTION(IntegerPredicate);
DECLARE_FUNCTION(EqualOp);
DECLARE_FUNCTION(LessOp);
DECLARE_FUNCTION(GreaterOp);
DECLARE_FUNCTION(LessEqualOp);
DECLARE_FUNCTION(GreaterEqualOp);
DECLARE_FUNCTION(MinOp);
DECLARE_FUNCTION(MaxOp);
DECLARE_FUNCTION(AbsOp);

// List functions
DECLARE_FUNCTION(PairPredicate);
DECLARE_FUNCTION(NullPredicate);
DECLARE_FUNCTION(ListPredicate);
DECLARE_FUNCTION(ConsOp);
DECLARE_FUNCTION(CarOp);
DECLARE_FUNCTION(CdrOp);
DECLARE_FUNCTION(ListOp);
DECLARE_FUNCTION(ListRef);
DECLARE_FUNCTION(ListTail);

// Boolean functions
DECLARE_FUNCTION(BooleanPredicate);
DECLARE_FUNCTION(NotOp);
DECLARE_FUNCTION(AndOp);
DECLARE_FUNCTION(OrOp);

// Variables functions
DECLARE_FUNCTION(DefineOp);
DECLARE_FUNCTION(SetOp);
DECLARE_FUNCTION(SymbolPredicate);
DECLARE_FUNCTION(SetCar);
DECLARE_FUNCTION(SetCdr);

// Control flow
DECLARE_FUNCTION(IfOp);
DECLARE_FUNCTION(LambdaOp);

#undef DECLARE_FUNCTION

struct Lambda : public Function {
    std::vector<ObjectPtr> commands;
    std::vector<std::string> arg_names;
    std::shared_ptr<Context> context;
    virtual ObjectPtr Apply(ObjectPtr args, std::shared_ptr<Context> context) const override;
};

#define REGISTER_KEYWORD(KEYWORD, FUNCTOR) {#KEYWORD, std::make_shared<FUNCTOR>()},

std::shared_ptr<Context> Context::GetKeywords() {
    static std::shared_ptr<Context> keywords = std::make_shared<Context>();
    if (keywords->name_table_.empty()) {
        keywords->name_table_ = {
            REGISTER_KEYWORD(+, PlusOp)
            REGISTER_KEYWORD(-, MinusOp)
            REGISTER_KEYWORD(*, MultiplyOp)
            REGISTER_KEYWORD(/, DivideOp)
            REGISTER_KEYWORD(number?, IntegerPredicate)
            REGISTER_KEYWORD(=, EqualOp)
            REGISTER_KEYWORD(<, LessOp)
            REGISTER_KEYWORD(>, GreaterOp)
            REGISTER_KEYWORD(<=, LessEqualOp)
            REGISTER_KEYWORD(>=, GreaterEqualOp)
            REGISTER_KEYWORD(min, MinOp)
            REGISTER_KEYWORD(max, MaxOp)
            REGISTER_KEYWORD(abs, AbsOp)
            REGISTER_KEYWORD(pair?, PairPredicate)
            REGISTER_KEYWORD(null?, NullPredicate)
            REGISTER_KEYWORD(list?, ListPredicate)
            REGISTER_KEYWORD(cons, ConsOp)
            REGISTER_KEYWORD(car, CarOp)
            REGISTER_KEYWORD(cdr, CdrOp)
            REGISTER_KEYWORD(list, ListOp)
            REGISTER_KEYWORD(list-ref, ListRef)
            REGISTER_KEYWORD(list-tail, ListTail)
            REGISTER_KEYWORD(boolean?, BooleanPredicate)
            REGISTER_KEYWORD(not, NotOp)
            REGISTER_KEYWORD(and, AndOp)
            REGISTER_KEYWORD(or, OrOp)
            REGISTER_KEYWORD(quote, QuoteOp)
            REGISTER_KEYWORD(define, DefineOp)
            REGISTER_KEYWORD(set!, SetOp)
            REGISTER_KEYWORD(set-car!, SetCar)
            REGISTER_KEYWORD(set-cdr!, SetCdr)
            REGISTER_KEYWORD(symbol?, SymbolPredicate)
            REGISTER_KEYWORD(if, IfOp)
            REGISTER_KEYWORD(lambda, LambdaOp)
        };
    }
    return keywords;
}

#undef REGISTER_KEYWORD

using std::make_shared;

namespace {
std::vector<ObjectPtr> VectorizeList(ObjectPtr list) {
    if (list == nullptr) {
        return {};
    }
    if (!Is<Cell>(list)) {
        throw RuntimeError("Expected list but got something else");
    }
    std::vector<ObjectPtr> result;
    while (true) {
        result.emplace_back(As<Cell>(list)->GetFirst());
        if (As<Cell>(list)->GetSecond() == nullptr) {
            return result;
        }
        if (!Is<Cell>(As<Cell>(list)->GetSecond())) {
            throw RuntimeError("Expected proper list but got improper one");
        }
        list = As<Cell>(list)->GetSecond();
    }
}
}  // namespace

ObjectPtr Function::Evaluate([[maybe_unused]] std::shared_ptr<Context> context) {
    throw RuntimeError{"Trying to evaluate a function-object itself"};
}

ObjectPtr QuoteOp::Apply(ObjectPtr args, [[maybe_unused]] std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("quote operator expects exactly one argument");
    }
    return arguments[0];
}

// TODO: add operator's name to this validator
#define VALIDATE_ARGUMENT_TYPE(ARGUMENT, TYPE)                                                     \
    (!Is<TYPE>(ARGUMENT) ? throw RuntimeError("Invalid type: expected " + std::string(#TYPE) +     \
                                              " but found" + std::string(typeid(ARGUMENT).name())) \
                         : 0)

ObjectPtr PlusOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    auto result = make_shared<Number>(0);
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result = make_shared<Number>(result->GetValue() + As<Number>(evaluated)->GetValue());
    }
    return result;
}

ObjectPtr MinusOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        throw RuntimeError("Minus operator expects at least one argument");
    }
    auto first_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(first_value, Number);
    if (arguments.size() == 1) {
        return make_shared<Number>(-As<Number>(first_value)->GetValue());
    }
    auto result = make_shared<Number>(As<Number>(first_value)->GetValue());
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result = make_shared<Number>(result->GetValue() - As<Number>(evaluated)->GetValue());
    }
    return result;
}

ObjectPtr MultiplyOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    auto result = make_shared<Number>(1);
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result = make_shared<Number>(result->GetValue() * As<Number>(evaluated)->GetValue());
    }
    return result;
}

ObjectPtr DivideOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        throw RuntimeError("Division operator expects at least one argument");
    }
    auto first_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(first_value, Number);
    if (arguments.size() == 1) {
        return make_shared<Number>(1 / As<Number>(first_value)->GetValue());
    }
    auto result = make_shared<Number>(As<Number>(first_value)->GetValue());
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result = make_shared<Number>(result->GetValue() / As<Number>(evaluated)->GetValue());
    }
    return result;
}

ObjectPtr IntegerPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Integer predicate expects exactly one argument");
    }
    return make_shared<Boolean>(Is<Number>(::Evaluate(arguments[0], context)));
}

ObjectPtr EqualOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() <= 1) {
        return make_shared<Boolean>(true);
    }
    auto first_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(first_value, Number);
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        if (As<Number>(first_value)->GetValue() != As<Number>(evaluated)->GetValue()) {
            return make_shared<Boolean>(false);
        }
    }
    return make_shared<Boolean>(true);
}

ObjectPtr LessOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() <= 1) {
        return make_shared<Boolean>(true);
    }
    auto previous_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(previous_value, Number);
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        if (!(As<Number>(previous_value)->GetValue() < As<Number>(evaluated)->GetValue())) {
            return make_shared<Boolean>(false);
        }
        previous_value = evaluated;
    }
    return make_shared<Boolean>(true);
}
ObjectPtr GreaterOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() <= 1) {
        return make_shared<Boolean>(true);
    }
    auto previous_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(previous_value, Number);
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        if (!(As<Number>(previous_value)->GetValue() > As<Number>(evaluated)->GetValue())) {
            return make_shared<Boolean>(false);
        }
        previous_value = evaluated;
    }
    return make_shared<Boolean>(true);
}
ObjectPtr LessEqualOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() <= 1) {
        return make_shared<Boolean>(true);
    }
    auto previous_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(previous_value, Number);
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        if (!(As<Number>(previous_value)->GetValue() <= As<Number>(evaluated)->GetValue())) {
            return make_shared<Boolean>(false);
        }
        previous_value = evaluated;
    }
    return make_shared<Boolean>(true);
}
ObjectPtr GreaterEqualOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() <= 1) {
        return make_shared<Boolean>(true);
    }
    auto previous_value = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(previous_value, Number);
    arguments.erase(arguments.begin());
    for (const auto& arg : arguments) {
        auto evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        if (!(As<Number>(previous_value)->GetValue() >= As<Number>(evaluated)->GetValue())) {
            return make_shared<Boolean>(false);
        }
        previous_value = evaluated;
    }
    return make_shared<Boolean>(true);
}

ObjectPtr MinOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        throw RuntimeError("Min-operator expects at least one argument");
    }
    auto evaluated = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(evaluated, Number);
    auto result = As<Number>(evaluated);
    for (const auto& arg : arguments) {
        evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result =
            make_shared<Number>(std::min(result->GetValue(), As<Number>(evaluated)->GetValue()));
    }
    return result;
}

ObjectPtr MaxOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        throw RuntimeError("Max-operator expects at least one argument");
    }
    auto evaluated = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(evaluated, Number);
    auto result = As<Number>(evaluated);
    for (const auto& arg : arguments) {
        evaluated = ::Evaluate(arg, context);
        VALIDATE_ARGUMENT_TYPE(evaluated, Number);
        result =
            make_shared<Number>(std::max(result->GetValue(), As<Number>(evaluated)->GetValue()));
    }
    return result;
}

ObjectPtr AbsOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("abs-operator expects exactly one argument");
    }
    auto evaluated = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(evaluated, Number);
    return make_shared<Number>(std::abs(As<Number>(evaluated)->GetValue()));
}

ObjectPtr BooleanPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Boolean predicate expects exactly one argument");
    }
    return make_shared<Boolean>(Is<Boolean>(::Evaluate(arguments[0], context)));
}

ObjectPtr NotOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Not-operator exactly one argument");
    }
    return make_shared<Boolean>(
        !make_shared<Boolean>(::Evaluate(arguments[0], context))->GetValue());
}

ObjectPtr AndOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        return make_shared<Boolean>(true);
    }
    ObjectPtr evaluated;
    for (const auto& arg : arguments) {
        evaluated = ::Evaluate(arg, context);
        if (!Boolean(evaluated).GetValue()) {
            return evaluated;
        }
    }
    return evaluated;
}

ObjectPtr OrOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        return make_shared<Boolean>(false);
    }
    ObjectPtr evaluated;
    for (const auto& arg : arguments) {
        evaluated = ::Evaluate(arg, context);
        if (Boolean(evaluated).GetValue()) {
            return evaluated;
        }
    }
    return make_shared<Boolean>(false);
}

ObjectPtr PairPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Pair predicate expects exactly one argument");
    }
    return make_shared<Boolean>(Is<Cell>(::Evaluate(arguments[0], context)));
}

ObjectPtr NullPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Null predicate expects exactly one argument");
    }
    return make_shared<Boolean>(::Evaluate(arguments[0], context) == nullptr);
}

ObjectPtr ListPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("List predicate expects exactly one argument");
    }
    auto ptr = ::Evaluate(arguments[0], context);
    while (Is<Cell>(ptr)) {
        ptr = As<Cell>(ptr)->GetSecond();
    }
    return make_shared<Boolean>(ptr == nullptr || ptr->Evaluate(context) == nullptr);
}

ObjectPtr ConsOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw RuntimeError("cons operator expects exactly 2 arguments");
    }
    return make_shared<Cell>(::Evaluate(arguments[0], context), ::Evaluate(arguments[1], context));
}

ObjectPtr CarOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("car operator expects exactly one argument");
    }
    auto evaluated = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(evaluated, Cell);
    return As<Cell>(evaluated)->GetFirst();
}

ObjectPtr CdrOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("cdr operator expects exactly one argument");
    }
    auto evaluated = ::Evaluate(arguments[0], context);
    VALIDATE_ARGUMENT_TYPE(evaluated, Cell);
    return As<Cell>(evaluated)->GetSecond();
}

ObjectPtr ListOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    if (args == nullptr) {
        return nullptr;
    }
    VALIDATE_ARGUMENT_TYPE(args, Cell);
    return make_shared<Cell>(As<Cell>(args)->GetFirst(),
                             Apply(As<Cell>(args)->GetSecond(), context));
}

ObjectPtr ListRef::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw RuntimeError("list-ref expects exactly 2 arguments");
    }
    auto eval_list = ::Evaluate(arguments[0], context);
    auto eval_ind = ::Evaluate(arguments[1], context);
    VALIDATE_ARGUMENT_TYPE(eval_ind, Number);
    auto list = VectorizeList(eval_list);
    if (As<Number>(eval_ind)->GetValue() < 0 ||
        static_cast<size_t>(As<Number>(eval_ind)->GetValue()) >= list.size()) {
        throw RuntimeError("list-ref index out of bounds");
    }
    return list[As<Number>(eval_ind)->GetValue()];
}

ObjectPtr ListTail::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw RuntimeError("list-tail expects exactly 2 arguments");
    }
    auto eval_list = ::Evaluate(arguments[0], context);
    auto eval_ind = ::Evaluate(arguments[1], context);
    VALIDATE_ARGUMENT_TYPE(eval_ind, Number);
    auto list = VectorizeList(eval_list);
    if (As<Number>(eval_ind)->GetValue() < 0 ||
        static_cast<size_t>(As<Number>(eval_ind)->GetValue()) > list.size()) {
        throw RuntimeError("list-tail index out of bounds");
    }
    auto result = eval_list;
    for (int i = 0; i < As<Number>(eval_ind)->GetValue(); ++i) {
        result = As<Cell>(result)->GetSecond();
    }
    return result;
}

ObjectPtr DefineOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.empty()) {
        throw SyntaxError("Empty define");
    }
    auto eval_name = arguments[0];
    if (!Is<Symbol>(eval_name)) {
        if (!Is<Cell>(eval_name)) {
            throw SyntaxError("");
        }
        // It is lambda definition
        auto name_args = VectorizeList(eval_name);
        auto real_name_obj = name_args[0];
        VALIDATE_ARGUMENT_TYPE(real_name_obj, Symbol);
        auto real_name = As<Symbol>(real_name_obj)->GetName();
        name_args.erase(name_args.begin());
        std::vector<ObjectPtr> arg_name_list = std::move(name_args);
        auto lambda_scope = make_shared<Context>(context);

        std::vector<std::string> arg_names;
        for (auto& arg_name : arg_name_list) {
            VALIDATE_ARGUMENT_TYPE(arg_name, Symbol);
            arg_names.emplace_back(As<Symbol>(arg_name)->GetName());
        }
        arguments.erase(arguments.begin());
        std::vector<ObjectPtr> commands = std::move(arguments);
        auto result = std::make_shared<Lambda>();
        result->commands = commands;
        result->arg_names = arg_names;
        result->context = lambda_scope;
        context->Define(real_name, result);
        return make_shared<Symbol>(real_name);
    }
    if (arguments.size() != 2) {
        throw SyntaxError("define expects exactly 2 arguments");
    }
    auto eval_val = ::Evaluate(arguments[1], context);

    context->Define(As<Symbol>(eval_name)->GetName(), eval_val);
    return make_shared<Symbol>(As<Symbol>(eval_name)->GetName());
}

ObjectPtr SetOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw SyntaxError("set! expects exactly 2 arguments");
    }
    auto eval_name = arguments[0];
    auto eval_val = ::Evaluate(arguments[1], context);
    VALIDATE_ARGUMENT_TYPE(eval_name, Symbol);

    auto ret = context->Get(As<Symbol>(eval_name)->GetName());
    context->Set(As<Symbol>(eval_name)->GetName(), eval_val);
    return ret;
}

ObjectPtr SetCar::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw SyntaxError("set-car! expects exactly 2 arguments");
    }
    auto eval_name = ::Evaluate(arguments[0], context);
    auto eval_val = ::Evaluate(arguments[1], context);

    VALIDATE_ARGUMENT_TYPE(eval_name, Cell);

    As<Cell>(eval_name)->SetFirst(eval_val);
    return nullptr;
}
ObjectPtr SetCdr::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2) {
        throw SyntaxError("set-car! expects exactly 2 arguments");
    }
    auto eval_name = ::Evaluate(arguments[0], context);
    auto eval_val = ::Evaluate(arguments[1], context);

    VALIDATE_ARGUMENT_TYPE(eval_name, Cell);

    As<Cell>(eval_name)->SetSecond(eval_val);
    return nullptr;
}

ObjectPtr SymbolPredicate::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 1) {
        throw RuntimeError("Symbol predicate expects exactly one argument");
    }
    return make_shared<Boolean>(Is<Symbol>(::Evaluate(arguments[0], context)));
}

ObjectPtr IfOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != 2 && arguments.size() != 3) {
        throw SyntaxError("Incorrect if statement");
    }
    auto eval_condition = ::Evaluate(arguments[0], context);
    if (make_shared<Boolean>(eval_condition)->GetValue()) {
        return ::Evaluate(arguments[1], context);
    }
    if (arguments.size() == 2) {
        return nullptr;
    }
    return ::Evaluate(arguments[2], context);
}

ObjectPtr LambdaOp::Apply(ObjectPtr args, std::shared_ptr<Context> context) const {
    auto lambda_scope = make_shared<Context>(context);
    auto argumets = VectorizeList(args);
    if (argumets.size() < 2) {
        throw SyntaxError("Invalid lambda expression");
    }
    auto arg_name_list = VectorizeList(argumets[0]);
    std::vector<std::string> arg_names;
    for (auto& arg_name : arg_name_list) {
        VALIDATE_ARGUMENT_TYPE(arg_name, Symbol);
        arg_names.emplace_back(As<Symbol>(arg_name)->GetName());
    }
    argumets.erase(argumets.begin());
    std::vector<ObjectPtr> commands = std::move(argumets);
    auto result = std::make_shared<Lambda>();
    result->commands = commands;
    result->arg_names = arg_names;
    result->context = lambda_scope;
    return result;
}

ObjectPtr Lambda::Apply(ObjectPtr args, std::shared_ptr<Context> contextp) const {
    auto arguments = VectorizeList(args);
    if (arguments.size() != arg_names.size()) {
        throw RuntimeError("Argument count is incorrect for lambda");
    }
    std::shared_ptr<Context> cur_context = std::make_shared<Context>(context);
    for (size_t i = 0; i < arguments.size(); ++i) {
        cur_context->Define(arg_names[i], ::Evaluate(arguments[i], contextp));
    }
    ObjectPtr last_result;
    for (auto& cmd : commands) {
        last_result = ::Evaluate(cmd, cur_context);
    }
    return last_result;
}
