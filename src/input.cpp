#include <rabbit/input.hpp>

#if RB_WINDOWS
#	include "drivers/win32/input_win32.hpp"
#endif

using namespace rb;

std::shared_ptr<input_impl> input::_impl;

void input::init() {
#if RB_WINDOWS
	_impl = std::make_shared<input_win32>();
#endif
}

void input::release() {
	_impl.reset();
}

void input::refresh() {
	_impl->refresh();
}

bool input::is_key_down(keycode key) {
	return _impl->is_key_down(key);
}

bool input::is_key_up(keycode key) {
	return _impl->is_key_up(key);
}

bool input::is_key_pressed(keycode key) {
	return _impl->is_key_pressed(key);
}

bool input::is_key_released(keycode key) {
	return _impl->is_key_released(key);
}

vec2i input::mouse_position() {
	return _impl->mouse_position();
}

float input::mouse_wheel() {
	return _impl->mouse_wheel();
}

bool input::is_mouse_button_down(mouse_button button) {
	return _impl->is_mouse_button_down(button);
}

bool input::is_mouse_button_up(mouse_button button) {
	return _impl->is_mouse_button_up(button);
}

bool input::is_mouse_button_pressed(mouse_button button) {
	return _impl->is_mouse_button_pressed(button);
}

bool input::is_mouse_button_released(mouse_button button) {
	return _impl->is_mouse_button_released(button);
}
