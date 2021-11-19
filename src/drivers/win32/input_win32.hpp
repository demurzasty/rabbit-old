#pragma once 

#include <rabbit/engine/platform/input.hpp>

#include <cstdint>

namespace rb {
	class input_win32 : public input_impl {
	public:
		void refresh() override;

		bool is_key_down(keycode key) override;

		bool is_key_up(keycode key) override;

		bool is_key_pressed(keycode key) override;

		bool is_key_released(keycode key) override;

		vec2i mouse_position() override;

		float mouse_wheel() override;

		bool is_mouse_button_down(mouse_button button) override;

		bool is_mouse_button_up(mouse_button button) override;

		bool is_mouse_button_pressed(mouse_button button) override;

		bool is_mouse_button_released(mouse_button button) override;

	private:
		std::uint8_t _last_key_state[256]{};
		std::uint8_t _key_state[256]{};
		std::uint8_t _last_mouse_state[8]{};
		std::uint8_t _mouse_state[8]{};
	};
}
