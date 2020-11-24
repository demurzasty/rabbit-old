#pragma once 

#include "vec2.hpp"

#include <cstdint>
#include <string>

namespace rb {
	struct config {
		struct {
			std::string title = "RabBit";
			vec2i size = { 1280, 720 };
			bool fullscreen = false;
			bool borderless = false;
			bool resizable = false;
		} window;

		long double fixed_time_step = 1.0L / 60.0L;
	};
}
