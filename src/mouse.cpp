#include <rabbit/mouse.hpp>

#if RB_PLATFORM_BACKEND_WIN32
#include "win32/mouse_win32.hpp"
#endif

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/mouse_sdl2.hpp"
#endif

using namespace rb;

std::shared_ptr<mouse> rb::make_mouse(const config& config, std::shared_ptr<window> window) {
#if RB_PLATFORM_BACKEND_WIN32
    if (config.platform_backend == platform_backend::win32) {
        return std::make_shared<mouse_win32>(window);
    }
#endif

#if RB_PLATFORM_BACKEND_SDL2
    if (config.platform_backend == platform_backend::sdl2) {
        return std::make_shared<mouse_sdl2>(window);
    }
#endif

    // todo: provide dummy
    return nullptr;
}
