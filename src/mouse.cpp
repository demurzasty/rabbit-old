#include <rabbit/mouse.hpp>

#if RB_PLATFORM_BACKEND_SDL2
#include "sdl2/mouse_sdl2.hpp"
namespace rb { using mouse_impl = mouse_sdl2; }
#else
#include "dummy/mouse_dummy.hpp"
namespace rb { using mouse_impl = mouse_dummy; }
#endif

using namespace rb;

std::shared_ptr<mouse> mouse::resolve(container& container) {
    return container.resolve<mouse_impl>();
}
