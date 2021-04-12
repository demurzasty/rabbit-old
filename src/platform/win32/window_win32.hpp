#pragma once 

#include <rabbit/platform/window.hpp>

#include <Windows.h>

namespace rb {
    class window_win32 : public window {
    public:
        window_win32(config& config);

        ~window_win32();

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

		virtual std::string title() const override;

		void show_cursor(bool enable) override;

		bool is_cursor_visible() const override;

		float mouse_wheel() const;

		const std::string& input_text() const;

		void close();

	private:
		HWND _hwnd{ nullptr };
		bool _open{ true };
    };
}