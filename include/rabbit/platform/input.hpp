#pragma once 

#include "../math/vec2.hpp"

#include <memory>

namespace rb {
	enum class keycode {
		a,
		b,
		c,
		d,
		e,
		f,
		g,
		h,
		i,
		j,
		k,
		l,
		m,
		n,
		o,
		p,
		q,
		r,
		s,
		t,
		u,
		v,
		w,
		x,
		y,
		z,

		num0,
		num1,
		num2,
		num3,
		num4,
		num5,
		num6,
		num7,
		num8,
		num9,

		apostrophe,
		minus,
		escape,
		left_control,
		left_shift,
		left_alt,
		left_system,
		right_control,
		right_shift,
		right_alt,
		right_system,
		menu,
		left_bracket,
		right_bracket,
		semicolon,
		comma,
		period,
		quote,
		slash,
		backslash,
		tilde,
		equal,
		hyphen,
		space,
		enter,
		backspace,
		tab,
		page_up,
		page_down,
		end,
		home,
		insert,
		del,
		add,
		subtract,
		multiply,
		divide,
		left,
		right,
		up,
		down,

		caps_lock,
		scroll_lock,
		num_lock,
		print_screen,

		numpad0,
		numpad1,
		numpad2,
		numpad3,
		numpad4,
		numpad5,
		numpad6,
		numpad7,
		numpad8,
		numpad9,

		f1,
		f2,
		f3,
		f4,
		f5,
		f6,
		f7,
		f8,
		f9,
		f10,
		f11,
		f12,
		f13,
		f14,
		f15,

		pause
	};

	enum class mouse_button {
		left,
		middle,
		right
	};

	class input_impl {
	public:
		virtual ~input_impl() = default;

		virtual void refresh() = 0;

		virtual bool is_key_down(keycode key) = 0;

		virtual bool is_key_up(keycode key) = 0;

		virtual bool is_key_pressed(keycode key) = 0;

		virtual bool is_key_released(keycode key) = 0;

		virtual vec2i mouse_position() = 0;

		virtual float mouse_wheel() = 0;

		virtual bool is_mouse_button_down(mouse_button button) = 0;

		virtual bool is_mouse_button_up(mouse_button button) = 0;

		virtual bool is_mouse_button_pressed(mouse_button button) = 0;

		virtual bool is_mouse_button_released(mouse_button button) = 0;
	};

	class input {
	public:
		static void init();

		static void release();

		static void refresh();

		static bool is_key_down(keycode key);

		static bool is_key_up(keycode key);

		static bool is_key_pressed(keycode key);

		static bool is_key_released(keycode key);

		static vec2i mouse_position();

		static float mouse_wheel();

		static bool is_mouse_button_down(mouse_button button);

		static bool is_mouse_button_up(mouse_button button);

		static bool is_mouse_button_pressed(mouse_button button);

		static bool is_mouse_button_released(mouse_button button);

	private:
		static std::shared_ptr<input_impl> _impl;
	};
}
