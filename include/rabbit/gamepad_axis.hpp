#pragma once 

#include <cstdint>

namespace rb {
	enum class gamepad_axis : std::int8_t {
		unknown = -1,

		left_x,
		left_y,
		right_x,
		right_y,
		left_trigger,
		right_trigger,

		count
	};
}
