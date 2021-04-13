#include "buffer_vulkan.hpp"

using namespace rb;

buffer_vulkan::buffer_vulkan(const buffer_desc& desc)
    : buffer(desc) {
}

buffer_vulkan::~buffer_vulkan() {
}

void* buffer_vulkan::map() {
    return nullptr;
}

void buffer_vulkan::unmap() {
}

void buffer_vulkan::update(const void* data, std::size_t size) {
}
