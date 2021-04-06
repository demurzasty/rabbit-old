#pragma once 

#include <string>
#include <stdexcept>

namespace rb {
    class exception : public std::runtime_error {
    public:
        exception(const std::string& message);
    };
}
