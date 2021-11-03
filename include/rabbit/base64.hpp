#pragma once 

#include "span.hpp"

#include <vector>
#include <string>
#include <cstdint>

namespace rb {
    class base64 {
    public:
        static std::string encode(const span<const std::uint8_t>& data);

        static std::vector<std::uint8_t> decode(const std::string& data);
    };
}
