#pragma once

#include "../core/di.hpp"
#include "../math/vec2.hpp"

#ifdef _WIN32
struct HWND__;
namespace rb { using window_handle = HWND__*; }
#else
namespace rb { using window_handle = void*; }
#endif

namespace rb {
    class window {
    public:
        /**
		 * @brief Install window implementation to dependency container.
		 */
		static void install(installer<window>& installer);

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
		 * @brief Sets window title.
		 */
		virtual void set_title(const std::string& title) = 0;

		/**
		 * @brief Retrieve window title.
		 *
		 * @return Window title.
		 */
		virtual std::string title() const = 0;
    };
}
