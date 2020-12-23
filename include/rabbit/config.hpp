#pragma once 

#include "vec2.hpp"

#include <cstdint>
#include <string>

namespace rb {
	enum class msaa : std::uint8_t {
		none = 1,
		x2 = 2,
		x4 = 4,
		x8 = 8,
		x16 = 16
	};

	struct config {
		struct {
			std::string title = "RabBit";
			vec2i size = { 1280, 720 };
			bool fullscreen = false;
			bool borderless = false;
			bool resizable = false;
			bool hide_cursor = false;
			msaa msaa = msaa::none;
		} window;

		struct {
			bool vsync = true;
		} graphics;

		long double fixed_time_step = 1.0L / 60.0L;
	};
}
