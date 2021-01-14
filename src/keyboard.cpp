#include <rabbit/keyboard.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/keyboard_sdl2.hpp"
namespace rb { using keyboard_impl = keyboard_sdl2; }
#else
#include "dummy/keyboard_dummy.hpp"
namespace rb { using keyboard_impl = keyboard_dummy; }
#endif

using namespace rb;

std::shared_ptr<keyboard> rb::make_keyboard(const config& config, std::shared_ptr<window> window) {
    return std::make_shared<keyboard_impl>(window);
}
