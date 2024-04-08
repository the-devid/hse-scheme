#include "object.h"

#include "error.h"

#include <memory>
#include <string>

Context::Context(std::shared_ptr<Context> upper) : upper_(upper) {
}

ObjectPtr Context::Get(const std::string& name) {
    auto current_context = this;
    while (current_context != nullptr && !current_context->name_table_.contains(name)) {
        current_context = current_context->upper_.get();
    }
    if (current_context == nullptr) {
        throw NameError("Unable to find symbol " + name);
    }
    return current_context->name_table_.at(name);
}
void Context::Set(const std::string& name, ObjectPtr value) {
    auto current_context = this;
    while (current_context != nullptr && !current_context->name_table_.contains(name)) {
        current_context = current_context->upper_.get();
    }
    if (current_context == nullptr) {
        throw NameError("Unable to find symbol " + name);
    }
    current_context->name_table_[name] = value;
}
void Context::Define(const std::string& name, ObjectPtr value) {
    name_table_[name] = value;
}

ObjectPtr Evaluate(ObjectPtr ptr, std::shared_ptr<Context> context) {
    if (ptr == nullptr) {
        throw RuntimeError("Empty list can not be evaluated");
    }
    return ptr->Evaluate(context);
}
std::string Serialize(ObjectPtr ptr) {
    if (ptr == nullptr) {
        return "()";
    }
    return ptr->Serialize();
}

Number::Number(int64_t number) : value_(number) {
}

int64_t Number::GetValue() const {
    return value_;
}

ObjectPtr Number::Evaluate([[maybe_unused]] std::shared_ptr<Context> context) {
    return this->shared_from_this();
}
std::string Number::Serialize() {
    return std::to_string(value_);
}

Boolean::Boolean(bool value) : value_(value) {
}

Boolean::Boolean(ObjectPtr obj) {
    if (Is<Boolean>(obj)) {
        value_ = As<Boolean>(obj)->GetValue();
    } else {
        value_ = true;
    }
}

bool Boolean::GetValue() const {
    return value_;
}

ObjectPtr Boolean::Evaluate([[maybe_unused]] std::shared_ptr<Context> context) {
    return this->shared_from_this();
}

std::string Boolean::Serialize() {
    return value_ ? "#t" : "#f";
}

Symbol::Symbol(const std::string& name) : name_(name) {
}

const std::string& Symbol::GetName() const {
    return name_;
}

ObjectPtr Symbol::Evaluate(std::shared_ptr<Context> context) {
    return context->Get(name_);
}
std::string Symbol::Serialize() {
    return name_;
}

ObjectPtr Cell::GetFirst() {
    return first_;
}
ObjectPtr Cell::GetSecond() {
    return second_;
}

Cell::Cell(ObjectPtr first, ObjectPtr second) : first_(first), second_(second) {
}

void Cell::SetFirst(ObjectPtr ptr) {
    first_ = ptr;
}
void Cell::SetSecond(ObjectPtr ptr) {
    second_ = ptr;
}

ObjectPtr Cell::Evaluate(std::shared_ptr<Context> context) {
    auto evaluated = ::Evaluate(first_, context);
    if (!Is<Function>(evaluated)) {
        throw RuntimeError("First element of list isn't applicable (not a function)");
    }
    return As<Function>(evaluated)->Apply(second_, context);
}
std::string Cell::Serialize() {
    std::string res = "(";
    auto current = this->shared_from_this();
    while (Is<Cell>(As<Cell>(current)->second_)) {
        res += ::Serialize(As<Cell>(current)->first_) + " ";
        current = As<Cell>(current)->second_;
    }
    if (As<Cell>(current)->second_ == nullptr) {
        res += ::Serialize(As<Cell>(current)->first_) + ")";
    } else {
        res += ::Serialize(As<Cell>(current)->first_) + " . " +
               ::Serialize(As<Cell>(current)->second_) + ")";
    }
    return res;
}
