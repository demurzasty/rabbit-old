#include <rabbit/window.hpp>
#include <rabbit/injector.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/window_sdl2.hpp"
namespace rb { using window_impl = window_sdl2; }
#elif RB_PLATFORM_BACKEND_WIN32
#include "win32/window_win32.hpp"
namespace rb { using window_impl = window_win32; }
#endif

using namespace rb;

std::shared_ptr<window> window::resolve(dependency_container& container) {
    return container.resolve<window_impl>();
}
