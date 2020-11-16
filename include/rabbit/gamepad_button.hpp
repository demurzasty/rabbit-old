#pragma once 

#include <cstdint>

namespace rb {
	enum class gamepad_button : std::int8_t {
		unknown = -1,

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
		triangle = y,

		count
	};
}
