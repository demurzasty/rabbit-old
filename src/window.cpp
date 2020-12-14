#include <rabbit/window.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/window_sdl2.hpp"
namespace rb { using window_impl = window_sdl2; }
#endif

using namespace rb;

std::shared_ptr<window> rb::make_window(config& config) {
    return std::make_shared<window_impl>(config);
}
