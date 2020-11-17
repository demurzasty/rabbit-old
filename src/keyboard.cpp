#include <rabbit/keyboard.hpp>

#if RB_PLATFORM_BACKEND_WIN32
#include "win32/keyboard_win32.hpp"
#endif

using namespace rb;

std::shared_ptr<keyboard> rb::make_keyboard(const config& config) {
#if RB_PLATFORM_BACKEND_WIN32
    if (config.platform_backend == platform_backend::win32) {
        return std::make_shared<keyboard_win32>();
    }
#endif

    // todo: provide dummy
    return nullptr;
}
