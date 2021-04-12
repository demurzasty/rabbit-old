#include <rabbit/platform/window.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/window_sdl2.hpp"
namespace rb { using window_impl = window_sdl2; }
#endif

using namespace rb;

void window::install(installer<window>& installer) {
    installer.install<window_impl>();
}
