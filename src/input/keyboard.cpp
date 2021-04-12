#include <rabbit/input/keyboard.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/keyboard_sdl2.hpp"
namespace rb { using keyboard_impl = keyboard_sdl2; }
#else
#include "dummy/keyboard_dummy.hpp"
namespace rb { using keyboard_impl = keyboard_dummy; }
#endif

using namespace rb;

void keyboard::install(installer<keyboard>& installer) {
    installer.install<keyboard_impl>();
}
