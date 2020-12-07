#pragma once 

#include <rabbit/window.hpp>

#include <glfw/glfw3.h>

namespace rb {
    class window_glfw : public window {
    public:
		window_glfw(config& config);

		~window_glfw();

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

		void show_cursor(bool enable) override;

		bool is_cursor_visible() const override;

		GLFWwindow* window() const;

    private:
        GLFWwindow* _window;
    };
}
