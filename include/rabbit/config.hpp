#pragma once 

#include "vec2.hpp"

#include <cstdint>
#include <string>

namespace rb {
	enum class window_backend : std::int8_t {
		unknown = -1,

		win32,
		glfw,
		cocoa,
		nswitch,

		count
	};

	enum class keyboard_backend : std::int8_t {
		unknown = -1,

		win32,

		count
	};

	enum class mouse_backend : std::int8_t {
		unknown = -1,

		win32,

		count
	};

	enum class gamepad_backend : std::int8_t {
		unknown = -1,

		xinput,
		dinput,

		count
	};

	enum class graphics_backend : std::int8_t {
		unknown = -1,

		directx11,
		opengl3,
		metal,

		count
	};

	enum class audio_backend : std::int8_t {
		unknown = -1,

		openal,
		xaudio,

		count
	};

	struct config {
		window_backend window_backend = window_backend::unknown;
		keyboard_backend keyboard_backend = keyboard_backend::unknown;
		mouse_backend mouse_backend = mouse_backend::unknown;
		gamepad_backend gamepad_backend = gamepad_backend::unknown;
		graphics_backend graphics_backend = graphics_backend::unknown;
		audio_backend audio_backend = audio_backend::unknown;

		std::string window_title = "RabBit";
		vec2i window_size = { 1280, 720 };
		bool fullscreen = false;
		bool window_borderless = false;
		bool window_resizable = false;
		long double fixed_time_step = 1.0L / 60.0L;

		config();

		config(const config&) = default;
		config(config&&) = default;
		config& operator=(const config&) = default;
		config& operator=(config&&) = default;
	};
}
