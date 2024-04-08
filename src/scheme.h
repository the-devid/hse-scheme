#pragma once

#include "object.h"

#include <memory>
#include <string>

class Interpreter {
public:
    Interpreter();

    std::string Run(const std::string&);

private:
    std::shared_ptr<Context> global_context_;
};
