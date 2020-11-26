#pragma once 

#include "enum.hpp"

namespace rb {
	RB_ENUM(texture_wrap, std::uint8_t, "clamp", "repeat")
	enum class texture_wrap : std::uint8_t {
		clamp,
		repeat
	};
}
