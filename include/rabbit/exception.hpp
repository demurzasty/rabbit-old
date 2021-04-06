#pragma once 

#include "format.hpp"

#include <string>
#include <stdexcept>

namespace rb {
    class exception : public std::runtime_error {
    public:
        exception(const std::string& message);
    };

    template<typename S, typename... Ts>
    inline exception make_exception(const S& format_str, Ts&&... args) {
        return format(format_str, std::forward<Ts>(args)...);
    }
}
