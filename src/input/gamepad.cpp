#include <rabbit/input/gamepad.hpp>

#if RB_GAMEPAD_BACKEND_SDL2
#include "sdl2/gamepad_sdl2.hpp"
namespace rb { using gamepad_impl = gamepad_sdl2; }
#elif RB_GAMEPAD_BACKEND_XINPUT
#include "xinput/gamepad_xinput.hpp"
namespace rb { using gamepad_impl = gamepad_xinput; }
#else
#include "dummy/gamepad_dummy.hpp"
namespace rb { using gamepad_impl = gamepad_dummy; }
#endif

using namespace rb;

void gamepad::install(installer<gamepad>& installer) {
    installer.install<gamepad_impl>();
}
