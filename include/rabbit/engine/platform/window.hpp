#pragma once 

#include <rabbit/engine/math/vec2.hpp>

#include <string>
#include <memory>

#if RB_WINDOWS
struct HWND__;
namespace rb { using window_handle = HWND__*; }
#else
namespace rb { using window_handle = void*; }
#endif


namespace rb {
	class window_impl {
	public:
		virtual ~window_impl() = default;

		virtual void poll_events() = 0;

		virtual window_handle native_handle() const = 0;

		virtual bool is_open() const = 0;

		virtual vec2u size() const = 0;

		virtual void set_title(const std::string& title) = 0;

		virtual std::string title() const = 0;
	};

	class window {
	public:
		static void init();

		static void release();

		static void poll_events();

		static window_handle native_handle();

		static bool is_open();

		static vec2u size();

		static void set_title(const std::string& title);

		static std::string title();

	private:
		static std::shared_ptr<window_impl> _impl;
	};
}