#pragma once 

#include "../core/enum.hpp"

namespace rb {
	RB_ENUM(mouse_button, std::uint8_t, "left", "middle", "right")
	enum class mouse_button : std::uint8_t {
		left,
		middle,
		right
	};
}
