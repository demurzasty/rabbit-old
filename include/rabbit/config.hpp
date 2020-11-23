#pragma once 

#include "vec2.hpp"

#include <cstdint>
#include <string>

namespace rb {
	struct config {
		std::string window_title = "RabBit";
		vec2i window_size = { 1280, 720 };
		bool fullscreen = false;
		bool window_borderless = false;
		bool window_resizable = false;
		long double fixed_time_step = 1.0L / 60.0L;
	};
}
