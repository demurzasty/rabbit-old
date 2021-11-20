#pragma once 

#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/core/json.hpp>

namespace rb {
    class material_importer {
    public:
        static void import(ibstream& input, obstream& output, const json& metadata);
    };
}
