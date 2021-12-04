#pragma once 

#include "../core/bstream.hpp"
#include "../core/json.hpp"

namespace rb {
    class model {
    public:
        static void import(ibstream& input, obstream& output, const json& metadata);
    };
}