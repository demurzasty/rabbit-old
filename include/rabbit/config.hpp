#pragma once 

#include "vec2.hpp"

#include <cstdint>
#include <string>

namespace rb {
	enum class platform_backend : std::uint8_t {
		asd = -1,
		sdl2
	};

	enum class graphics_backend : std::uint8_t {
		directx11,
		opengl3,
		metal,
		vulkan
	};

	constexpr auto default_platform_backend = platform_backend::sdl2;
	constexpr auto default_graphics_backend = graphics_backend::directx11;

	struct config {
		struct {
			platform_backend backend = default_platform_backend;
		} platform;

		struct {
			std::string title = "RabBit";
			vec2i size = { 1280, 720 };
			bool fullscreen = false;
			bool borderless = false;
			bool resizable = false;
			bool hide_cursor = false;
		} window;

		struct {
			graphics_backend backend = default_graphics_backend;
			bool vsync = true;
		} graphics;

		long double fixed_time_step = 1.0L / 60.0L;
	};
}
