#include <rabbit/window.hpp>

#if RB_PLATFORM_BACKEND_WIN32
#include "win32/window_win32.hpp"
#endif

using namespace rb;

std::shared_ptr<window> rb::make_window(config& config) {
#if RB_PLATFORM_BACKEND_WIN32
    if (config.platform_backend == platform_backend::win32) {
        return std::make_shared<window_win32>(config);
    }
#endif

    // todo: provide dummy
    return nullptr;
}
