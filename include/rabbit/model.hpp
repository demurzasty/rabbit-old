#pragma once 

#include "json.hpp"
#include "bstream.hpp"

#include <string>

namespace rb {
    class model {
    public:
        static void import(const std::string& input, bstream& output, const json& metadata);
    };
}