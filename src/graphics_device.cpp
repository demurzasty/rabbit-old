#include <rabbit/graphics_device.hpp>

#if RB_GRAPHICS_BACKEND_VULKAN
#include "vulkan/graphics_device_vk.hpp"
namespace rb { using graphics_device_impl = graphics_device_vk; }
#elif RB_GRAPHICS_BACKEND_DIRECTX
#include "dx11/graphics_device_dx11.hpp"
namespace rb { using graphics_device_impl = graphics_device_dx11; }
#elif RB_GRAPHICS_BACKEND_OPENGL
#include "ogl3/graphics_device_ogl3.hpp"
namespace rb { using graphics_device_impl = graphics_device_ogl3; }
#endif

using namespace rb;

std::shared_ptr<graphics_device> rb::make_graphics_device(const config& config, std::shared_ptr<window> window) {
    return std::make_shared<graphics_device_impl>(config, window);
}
