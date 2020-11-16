#pragma once 

#include "config.hpp"

#include <string>
#include <memory>

#if RB_WINDOW_BACKEND_WIN32
struct HWND__;
namespace rb { typedef HWND__* window_handle; }
#endif

namespace rb {
    class window {
    public:
		/**
		 * @brief Default virtual destructor.
		 */
		virtual ~window() = default;

		/**
		 * @brief Tell whether window is open.
		 *
		 * @return True if window is open.
		 */
		virtual bool is_open() const = 0;

		/**
		 * @brief Returns native window handle.
		 *
		 * @return Native window handle.
		 */
		virtual window_handle native_handle() const = 0;

		/**
		 * @brief Poll window events.
		 *
		 * @return True if event queue was not empty.
		 */
		virtual void poll_events() = 0;

		/**
		 * @brief Returns window size.
		 *
		 * @return Window size.
		 */
		virtual vec2i size() const = 0;

		/**
		 * @brief Maximize window.
		 */
		virtual void maximize() = 0;

		/**
		 * @brief Sets windows resizable flag.
		 */
		virtual void set_resizable(bool resizable) const = 0;

		/**
		 * @brief Tell whether window is resizable.
		 *
		 * @return True if resizable.
		 */
		virtual bool is_resizable() const = 0;

		/**
		 * @brief Tell whether window is focused.
		 */
		virtual bool is_focused() const = 0;

		virtual void set_title(const std::string& title) = 0;

		virtual std::string title() const = 0;
    };

	/**
	 * @brief Makes new instance of platform independent window.
	 */
	std::shared_ptr<window> make_window(config& config);
}
