#include <rabbit/mouse.hpp>

#if RB_PLATFORM_BACKEND_WIN32
#include "win32/mouse_win32.hpp"
namespace rb { using mouse_native = mouse_win32; }
#endif

using namespace rb;

std::shared_ptr<mouse> rb::make_mouse(const config& config, std::shared_ptr<window> window) {
#if RB_PLATFORM_BACKEND_WIN32
    if (config.platform_backend == platform_backend::win32) {
        return std::make_shared<mouse_native>(window);
    }
#endif

    // todo: provide dummy
    return nullptr;
}
