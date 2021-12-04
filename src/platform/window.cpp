#include <rabbit/platform/window.hpp>

#if RB_WINDOWS
#	include "../drivers/win32/window_win32.hpp"
#endif

using namespace rb;

std::shared_ptr<window_impl> window::_impl;

void window::init() {
#if RB_WINDOWS
	_impl = std::make_shared<window_win32>();
#endif
}

void window::release() {
	_impl.reset();
}

void window::poll_events() {
	_impl->poll_events();
}

window_handle window::native_handle() {
	return _impl->native_handle();
}

bool window::is_open() {
	return _impl->is_open();
}

vec2u window::size() {
	return _impl->size();
}

void window::set_title(const std::string& title) {
	_impl->set_title(title);
}

std::string window::title() {
	return _impl->title();
}