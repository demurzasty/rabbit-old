#pragma once 

#include "json.hpp"

#include <string>

namespace rb {
    class model {
    public:
        static void import(const std::string& input, const std::string& output, const json& metadata);
    };
}