#include "buffer_vk.hpp"

#include <rabbit/exception.hpp>

#include <map>

using namespace rb;

buffer_vk::buffer_vk(const buffer_desc& desc)
    : buffer(desc) {
}

buffer_vk::~buffer_vk() {
}

void* buffer_vk::map() {
    return nullptr;
}

void buffer_vk::unmap() {
}

void buffer_vk::update(const void* data, std::size_t size) {
    if (!is_mutable()) {
        throw exception{ "[VK] Buffer need to be mutable to update content" };
    }

    if (size > this->size()) {
        throw exception{ "[VK] Overflow data" };
    }
}
