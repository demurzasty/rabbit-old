#pragma once 

#include "enum.hpp"

namespace rb {
	RB_ENUM(mouse_button, std::uint8_t, "left", "middle", "right")
	enum class mouse_button : std::uint8_t {
		left,
		middle,
		right
	};
}
