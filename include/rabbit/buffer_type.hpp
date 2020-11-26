#pragma once 

#include "enum.hpp"

namespace rb {
	RB_ENUM(buffer_type, std::uint8_t, "vertex", "index")
    enum class buffer_type : std::uint8_t {
        vertex,
        index
    };
}