#include <rabbit/keyboard.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/keyboard_sdl2.hpp"
namespace rb { using keyboard_impl = keyboard_sdl2; }
#elif RB_PLATFORM_BACKEND_WIN32
#include "win32/keyboard_win32.hpp"
namespace rb { using keyboard_impl = keyboard_win32; }
#endif

using namespace rb;

std::shared_ptr<keyboard> rb::make_keyboard(const config& config) {
    return std::make_shared<keyboard_impl>();
}
