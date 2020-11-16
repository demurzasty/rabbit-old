#include <rabbit/keyboard.hpp>

#if RB_KEYBOARD_BACKEND_WIN32
#include "win32/keyboard_win32.hpp"
#endif

using namespace rb;

std::shared_ptr<keyboard> rb::make_keyboard(const config& config) {
#if RB_KEYBOARD_BACKEND_WIN32
    if (config.keyboard_backend == keyboard_backend::win32) {
        return std::make_shared<keyboard_win32>();
    }
#endif

    // todo: provide dummy
    return nullptr;
}
