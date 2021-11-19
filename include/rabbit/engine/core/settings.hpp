#pragma once 

#include <rabbit/engine/core/version.hpp>
#include <rabbit/engine/graphics/graphics.hpp>
#include <rabbit/engine/math/vec2.hpp>

#include <string>

namespace rb {
	class settings {
	public:
		static version app_version;
		static std::string window_title;
		static vec2u window_size;
		static bool fullscreen;
		static graphics_backend graphics_backend;
		static bool vsync;
	};
}
