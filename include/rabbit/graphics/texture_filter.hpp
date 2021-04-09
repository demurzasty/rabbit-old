#pragma once 

#include "../core/enum.hpp"

namespace rb {
	RB_ENUM(texture_filter, std::uint8_t, "nearest", "linear")
	enum class texture_filter : std::uint8_t {
		nearest,
		linear
	};
}
