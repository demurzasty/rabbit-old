#pragma once 

#include <rabbit/bstream.hpp>
#include <rabbit/json.hpp>

namespace rb {
    class model {
    public:
        static void import(ibstream& input, obstream& output, const json& metadata);
    };
}