#pragma once 

#include "../core/enum.hpp"

namespace rb {
	RB_ENUM(gamepad_axis, std::uint8_t, "left_x", "left_y", "right_x", "right_y", "left_trigger", "right_trigger")
	enum class gamepad_axis : std::uint8_t {
		left_x,
		left_y,
		right_x,
		right_y,
		left_trigger,
		right_trigger
	};
}
