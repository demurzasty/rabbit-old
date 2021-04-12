#include <rabbit/graphics/graphics_device.hpp>

#if RB_GRAPHICS_BACKEND_DIRECTX
#include "dx11/graphics_device_dx11.hpp"
namespace rb { using graphics_device_impl = graphics_device_dx11; }
#elif RB_GRAPHICS_BACKEND_OPENGL
#include "ogl3/graphics_device_ogl3.hpp"
namespace rb { using graphics_device_impl = graphics_device_ogl3; }
#endif

using namespace rb;

void graphics_device::install(installer<graphics_device>& installer) {
    installer.install<graphics_device_impl>();
}
