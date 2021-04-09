#include <rabbit/input/keyboard.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/keyboard_sdl2.hpp"
namespace rb { using keyboard_impl = keyboard_sdl2; }
#else
#include "dummy/keyboard_dummy.hpp"
namespace rb { using keyboard_impl = keyboard_dummy; }
#endif

using namespace rb;

std::shared_ptr<keyboard> keyboard::resolve(container& container) {
    return container.resolve<keyboard_impl>();
}
