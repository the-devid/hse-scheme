#pragma once

#include "error.h"
#include <memory>
#include <string>
#include <unordered_map>

class Context;

class Object : public std::enable_shared_from_this<Object> {
public:
    virtual ~Object() = default;
    virtual std::shared_ptr<Object> Evaluate([[maybe_unused]] std::shared_ptr<Context> context) {
        throw RuntimeError("Unimplemented evaluation of Object");
    }
    virtual std::string Serialize() {
        throw RuntimeError("Unimplemented serialization of Object");
    }
};

// We want all objects to be mutable.
using ObjectPtr = std::shared_ptr<Object>;

class Context {
public:
    Context() = default;
    Context(std::shared_ptr<Context> upper);

    ObjectPtr Get(const std::string& name);
    void Set(const std::string& name, ObjectPtr value);
    void Define(const std::string& name, ObjectPtr value);

    static std::shared_ptr<Context> GetKeywords();

    std::unordered_map<std::string, ObjectPtr> GetNameTable() {
        return name_table_;
    }
    void SetNameTable(std::unordered_map<std::string, ObjectPtr> name_table) {
        name_table_ = name_table;
    }
    ObjectPtr StraightGet(const std::string& name) {
        return name_table_.contains(name) ? name_table_[name] : std::make_shared<Object>();
    }

private:
    std::unordered_map<std::string, ObjectPtr> name_table_;
    std::shared_ptr<Context> upper_ = nullptr;
};

//! Function that either calls a method or throwss if argument is nullptr.
ObjectPtr Evaluate(ObjectPtr ptr, std::shared_ptr<Context> context);
//! Function that either calls a method or returns `()` if argument is nullptr.
std::string Serialize(ObjectPtr ptr);

class Number : public Object {
public:
    Number(int64_t value);

    int64_t GetValue() const;
    virtual ObjectPtr Evaluate(std::shared_ptr<Context> context) override;
    virtual std::string Serialize() override;

private:
    int64_t value_;
};

class Boolean : public Object {
public:
    Boolean(bool value);

    explicit Boolean(ObjectPtr obj);

    bool GetValue() const;
    virtual ObjectPtr Evaluate(std::shared_ptr<Context> context) override;
    virtual std::string Serialize() override;

private:
    bool value_;
};

class Symbol : public Object {
public:
    Symbol(const std::string& name);

    const std::string& GetName() const;
    virtual ObjectPtr Evaluate(std::shared_ptr<Context> context) override;
    virtual std::string Serialize() override;

private:
    std::string name_;
};

class Cell : public Object {
public:
    Cell() = default;
    Cell(ObjectPtr first, ObjectPtr second);

    ObjectPtr GetFirst();
    ObjectPtr GetSecond();

    void SetFirst(ObjectPtr);
    void SetSecond(ObjectPtr);

    virtual ObjectPtr Evaluate(std::shared_ptr<Context> context) override;
    virtual std::string Serialize() override;

private:
    ObjectPtr first_;
    ObjectPtr second_;
};

template <class T>
std::shared_ptr<T> As(const ObjectPtr& obj) {
    return std::dynamic_pointer_cast<T>(obj);
}

template <class T>
bool Is(const ObjectPtr& obj) {
    return std::dynamic_pointer_cast<T>(obj) != nullptr;
}

struct Function : public Object {
    virtual ObjectPtr Apply(ObjectPtr args, std::shared_ptr<Context> context) const = 0;
    virtual ObjectPtr Evaluate(std::shared_ptr<Context> context) override;
};
