#pragma once 

#include "../core/enum.hpp"

namespace rb {
	RB_ENUM(gamepad_button, std::uint8_t, "a", "b", "x", "y", "left_bumper", "right_bumper", "left_thumb", "right_thumb",
		"back", "start", "dpad_up", "dpad_right", "dpad_down", "dpad_left")
	enum class gamepad_button : std::uint8_t {
		a,
		b,
		x,
		y,

		left_bumper,
		right_bumper,
		left_thumb,
		right_thumb,

		back,
		start,

		dpad_up,
		dpad_right,
		dpad_down,
		dpad_left,

		cross = a,
		circle = b,
		square = x,
		triangle = y
	};
}
