#include <rabbit/graphics_device.hpp>

#if RB_GRAPHICS_BACKEND_DIRECTX
#include "dx11/graphics_device_dx11.hpp"
#endif

#if RB_GRAPHICS_BACKEND_OPENGL
#include "ogl3/graphics_device_ogl3.hpp"
#endif

using namespace rb;

std::shared_ptr<graphics_device> rb::make_graphics_device(const config& config, std::shared_ptr<window> window) {
#if RB_GRAPHICS_BACKEND_DIRECTX
    if (config.graphics_backend == graphics_backend::directx11) {
        return std::make_shared<graphics_device_dx11>(config, window);
    }
#endif 

#if RB_GRAPHICS_BACKEND_OPENGL
    if (config.graphics_backend == graphics_backend::opengl3) {
        return std::make_shared<graphics_device_ogl3>(config, window);
    }
#endif

    return nullptr;
}
