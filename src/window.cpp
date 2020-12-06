#include <rabbit/window.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/window_sdl2.hpp"
namespace rb { using window_impl = window_sdl2; }
#elif RB_PLATFORM_BACKEND_WIN32
#include "win32/window_win32.hpp"
namespace rb { using window_impl = window_win32; }
#elif RB_PLATFORM_BACKEND_GLFW
#include "glfw/window_glfw.hpp"
namespace rb { using window_impl = window_glfw; }
#endif

using namespace rb;

std::shared_ptr<window> rb::make_window(config& config) {
    return std::make_shared<window_impl>(config);
}
