#pragma once 

#include <string>
#include <stdexcept>

namespace rb {
    class exception : public std::logic_error {
    public:
        exception(const std::string& message);
    };
}
