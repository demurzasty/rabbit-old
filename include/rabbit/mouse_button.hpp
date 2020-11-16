#pragma once 

#include <cstdint>

namespace rb {
	enum class mouse_button : std::int8_t {
		unknown = -1,

		left,
		middle,
		right,

		count
	};
}
