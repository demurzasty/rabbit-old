#include <rabbit/graphics/graphics_device.hpp>

#if RB_VULKAN
#include "vulkan/graphics_device_vulkan.hpp"
namespace rb { using graphics_device_impl = graphics_device_vulkan; }
#endif

using namespace rb;

void graphics_device::install(installer<graphics_device>& installer) {
    installer.install<graphics_device_impl>();
}
