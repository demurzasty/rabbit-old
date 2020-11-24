#pragma once 

#include <rabbit/window.hpp>

#include <SDL.h>
#include <SDL_syswm.h>

namespace rb {
	class window_sdl2 : public window {
	public:
		window_sdl2(config& config);

		~window_sdl2();

		bool is_open() const override;

		window_handle native_handle() const override;

		void swap_buffers() override;

		void poll_events() override;

		vec2i size() const override;

		void maximize() override;

		void set_resizable(bool resizable) const override;

		bool is_resizable() const override;

		bool is_focused() const override;

		void set_title(const std::string& title) override;

		std::string title() const override;

	private:
		SDL_Window* _window = nullptr;
		bool _open = true;

#if RB_GRAPHICS_BACKEND_OPENGL
		SDL_GLContext _context = nullptr;
#endif
	};
}
