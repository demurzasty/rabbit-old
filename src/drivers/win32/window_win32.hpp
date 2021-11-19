#pragma once 

#include <rabbit/engine/platform/window.hpp>
#include <rabbit/engine/core/settings.hpp>

#include <Windows.h>

namespace rb {
	class window_win32 : public window_impl {
	public:
		window_win32();

		~window_win32();

		bool is_open() const override;

		window_handle native_handle() const override;

		void poll_events() override;

		vec2u size() const override;

		void set_title(const std::string& title) override;

		std::string title() const override;

		void close();

	private:
		HWND _hwnd{ nullptr };
		bool _open{ true };
	};
}
